#include <QtTest>
#include <QBuffer>
#include <QCryptographicHash>
#include <future>
#include <algorithm>
#include <stdexcept>
#include <vector>
#include "rbl_hash_sha256.h"
#ifdef RANGE_SHA256_TEST_FAULTS
void rHashSha256TestFault(int);
#endif
class BrokenDevice : public QIODevice {
public:
    BrokenDevice() { open(ReadOnly); }
    bool atEnd() const override { return false; }
protected:
    qint64 readData(char *,qint64) override { return -1; }
    qint64 writeData(const char *,qint64) override { return -1; }
};
class TestHashSha256 : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
#ifdef RANGE_SHA256_TEST_FAULTS
        rHashSha256TestFault(1);
        QVERIFY_EXCEPTION_THROWN(RHashSha256(),std::runtime_error);
#endif
    }
    void vectorsAndChunks() {
        QCOMPARE(RHashSha256::hash(QByteArrayView()).toHex(),QByteArray("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));
        QCOMPARE(RHashSha256::hash(QByteArrayLiteral("abc")).toHex(),QByteArray("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
        const QList<int> sizes{0,1,55,56,63,64,65,119,120,127,128,129,3486,3*1024*1024+17};
        for(auto size:sizes) {
            QByteArray bytes(size,Qt::Uninitialized);
            for(int i=0;i<size;++i)bytes[i]=char((i*137+size)&255);
            const auto expected=QCryptographicHash::hash(bytes,QCryptographicHash::Sha256);
            QCOMPARE(RHashSha256::hash(bytes),expected);
            for(auto chunk:QList<int>{1,7,64,65536}) {
                if(size>65536 && chunk<64)continue;
                RHashSha256 stream;
                stream.addData(QByteArrayView());
                for(qsizetype pos=0;pos<bytes.size();pos+=chunk)stream.addData(QByteArrayView(bytes).sliced(pos,std::min<qsizetype>(chunk,bytes.size()-pos)));
                QCOMPARE(stream.result(),expected);
                QCOMPARE(stream.result(),expected);
                QVERIFY_EXCEPTION_THROWN(stream.addData(QByteArrayView()),std::logic_error);
            }
        }
        const QByteArray zero(4096,'\0');
        QCOMPARE(RHashSha256::hash(zero),QCryptographicHash::hash(zero,QCryptographicHash::Sha256));
    }
    void independentStreamsAndReuse() {
        RHashSha256 left,right;
        QByteArray allLeft,allRight;
        for(int i=0;i<20001;++i) {
            QByteArray bytes=QByteArray::number(i)+QByteArray(31,char(i&255));
            left.addData(bytes);allLeft+=bytes;
            QCOMPARE(RHashSha256::hash(bytes),QCryptographicHash::hash(bytes,QCryptographicHash::Sha256));
            right.addData(QByteArrayView(bytes).sliced(1));allRight+=bytes.mid(1);
            QCOMPARE(RHashSha256::hash(QByteArrayView()),QCryptographicHash::hash(QByteArrayView(),QCryptographicHash::Sha256));
        }
        QCOMPARE(left.result(),QCryptographicHash::hash(allLeft,QCryptographicHash::Sha256));
        QCOMPARE(right.result(),QCryptographicHash::hash(allRight,QCryptographicHash::Sha256));
    }
    void threadsAndLifetime() {
        std::vector<std::future<bool>> workers;
        for(int thread=0;thread<8;++thread)workers.emplace_back(std::async(std::launch::async,[thread] {
            for(int i=0;i<1000;++i) {
                QByteArray bytes(3486,char((thread+i)&255));
                const auto expected=QCryptographicHash::hash(bytes,QCryptographicHash::Sha256);
                RHashSha256 stream;stream.addData(QByteArrayView(bytes).first(57));
                if(RHashSha256::hash(bytes)!=expected)return false;
                stream.addData(QByteArrayView(bytes).sliced(57));
                if(stream.result()!=expected)return false;
            }
            return true;
        }));
        for(auto &worker:workers)QVERIFY(worker.get());
    }
    void deviceFailureAndPosition() {
        QBuffer device;device.setData("skip\0payload\xff",13);QVERIFY(device.open(QIODevice::ReadOnly));QVERIFY(device.seek(4));
        RHashSha256 hash;QVERIFY(hash.addData(&device));
        QCOMPARE(hash.result(),QCryptographicHash::hash(device.data().mid(4),QCryptographicHash::Sha256));
        RHashSha256 bad;BrokenDevice broken;QVERIFY(!bad.addData(&broken));
        QVERIFY_EXCEPTION_THROWN(bad.result(),std::runtime_error);
        QVERIFY_EXCEPTION_THROWN(bad.addData(QByteArrayLiteral("abc")),std::runtime_error);
        RHashSha256 absent;QVERIFY(!absent.addData(nullptr));QVERIFY_EXCEPTION_THROWN(absent.result(),std::runtime_error);
    }
    void cryptoFailureIsClosed() {
#ifdef RANGE_SHA256_TEST_FAULTS
        rHashSha256TestFault(2);QVERIFY_EXCEPTION_THROWN(RHashSha256(),std::runtime_error);
        for(int operation:{3,4}) {
            RHashSha256 stream;
            stream.addData(QByteArrayLiteral("prefix"));rHashSha256TestFault(operation);
            if(operation==3)QVERIFY_EXCEPTION_THROWN(stream.addData(QByteArrayLiteral("suffix")),std::runtime_error);
            else QVERIFY_EXCEPTION_THROWN(stream.result(),std::runtime_error);
            QVERIFY_EXCEPTION_THROWN(stream.result(),std::runtime_error);
            QVERIFY_EXCEPTION_THROWN(stream.addData(QByteArrayLiteral("later")),std::runtime_error);
            rHashSha256TestFault(operation);QVERIFY_EXCEPTION_THROWN(RHashSha256::hash(QByteArrayLiteral("failed")),std::runtime_error);
            QCOMPARE(RHashSha256::hash(QByteArrayLiteral("abc")).toHex(),QByteArray("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
        }
#endif
    }
};
QTEST_APPLESS_MAIN(TestHashSha256)
#include "tst_hash_sha256.moc"
