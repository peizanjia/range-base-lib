#include "rbl_hash_sha256.h"
#include <QIODevice>
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <vector>

#if defined(Q_OS_WIN) && !defined(RANGE_SHA256_TEST_QT_BACKEND)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <bcrypt.h>
#else
#include <QCryptographicHash>
#endif

namespace {
#if defined(Q_OS_WIN) && !defined(RANGE_SHA256_TEST_QT_BACKEND)
#ifdef RANGE_SHA256_TEST_FAULTS
thread_local int faultOperation = 0;
bool inject(int operation) { if (faultOperation != operation) return false; faultOperation = 0; return true; }
#else
bool inject(int) { return false; }
#endif
void check(NTSTATUS status, const char *operation)
{
    if (status < 0)
        throw std::runtime_error(QStringLiteral("%1 failed: NTSTATUS 0x%2")
            .arg(QString::fromLatin1(operation)).arg(quint32(status),8,16,QLatin1Char('0')).toStdString());
}
constexpr NTSTATUS injectedFailure = static_cast<NTSTATUS>(0xC0000001L);
struct Provider
{
    BCRYPT_ALG_HANDLE handle = nullptr;
    ULONG objectBytes = 0;
    Provider()
    {
        check(inject(1) ? injectedFailure : BCryptOpenAlgorithmProvider(&handle,BCRYPT_SHA256_ALGORITHM,MS_PRIMITIVE_PROVIDER,BCRYPT_HASH_REUSABLE_FLAG),"BCryptOpenAlgorithmProvider");
        try {
            ULONG returned = 0, digestBytes = 0;
            check(BCryptGetProperty(handle,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&objectBytes),sizeof(objectBytes),&returned,0),"BCryptGetProperty(object)");
            if (returned != sizeof(objectBytes) || objectBytes == 0) throw std::runtime_error("Invalid SHA-256 object size");
            check(BCryptGetProperty(handle,BCRYPT_HASH_LENGTH,reinterpret_cast<PUCHAR>(&digestBytes),sizeof(digestBytes),&returned,0),"BCryptGetProperty(digest)");
            if (returned != sizeof(digestBytes) || digestBytes != 32) throw std::runtime_error("Invalid SHA-256 digest size");
        } catch (...) { BCryptCloseAlgorithmProvider(handle,0); handle=nullptr; throw; }
    }
    ~Provider() { if(handle) BCryptCloseAlgorithmProvider(handle,0); }
};
std::shared_ptr<const Provider> provider()
{
    static const auto value = std::make_shared<const Provider>();
    return value;
}
class Engine
{
    // Keep provider alive until after hash destruction, including TLS teardown.
    std::shared_ptr<const Provider> algorithm = provider();
    std::vector<UCHAR> object = std::vector<UCHAR>(algorithm->objectBytes);
    BCRYPT_HASH_HANDLE handle = nullptr;
public:
    explicit Engine(bool reusable = false)
    {
        check(inject(2) ? injectedFailure : BCryptCreateHash(algorithm->handle,&handle,object.data(),ULONG(object.size()),nullptr,0,reusable?BCRYPT_HASH_REUSABLE_FLAG:0),"BCryptCreateHash");
    }
    ~Engine() { if(handle) BCryptDestroyHash(handle); }
    Engine(const Engine &) = delete;
    Engine &operator=(const Engine &) = delete;
    void update(QByteArrayView bytes)
    {
        while (!bytes.empty()) {
            const auto count = std::min<quint64>(quint64(bytes.size()),std::numeric_limits<ULONG>::max());
            check(inject(3) ? injectedFailure : BCryptHashData(handle,reinterpret_cast<PUCHAR>(const_cast<char*>(bytes.data())),ULONG(count),0),"BCryptHashData");
            bytes = bytes.sliced(qsizetype(count));
        }
    }
    QByteArray finish()
    {
        QByteArray bytes(32,Qt::Uninitialized);
        check(inject(4) ? injectedFailure : BCryptFinishHash(handle,reinterpret_cast<PUCHAR>(bytes.data()),32,0),"BCryptFinishHash");
        return bytes;
    }
};
#else
class Engine
{
    QCryptographicHash hash{QCryptographicHash::Sha256};
public:
    void update(QByteArrayView bytes) { hash.addData(bytes); }
    QByteArray finish() { auto bytes=hash.result(); if(bytes.size()!=32)throw std::runtime_error("Invalid Qt SHA-256 digest size");return bytes; }
};
#endif
}

#ifdef RANGE_SHA256_TEST_FAULTS
// Test-only symbol: not declared in the installed public header or built in production.
void rHashSha256TestFault(int operation) { faultOperation = operation; }
#endif

class RHashSha256::Impl
{
public:
    Engine engine;
    bool failed = false;
    QByteArray digest;
    void open() const {
        if(failed)throw std::runtime_error("SHA-256 stream has failed");
        if(!digest.isEmpty())throw std::logic_error("SHA-256 stream is finalized");
    }
};
RHashSha256::RHashSha256() : impl(std::make_unique<Impl>()) {}
RHashSha256::~RHashSha256() = default;
void RHashSha256::addData(QByteArrayView bytes)
{
    impl->open();
    try { impl->engine.update(bytes); } catch (...) { impl->failed=true;throw; }
}
bool RHashSha256::addData(QIODevice *device)
{
    impl->open();
    if(!device || !device->isOpen() || !device->isReadable()) { impl->failed=true;return false; }
    char buffer[64*1024];
    for (;;) {
        const auto count=device->read(buffer,sizeof(buffer));
        if(count < 0 || (count == 0 && !device->atEnd())) { impl->failed=true;return false; }
        if(count == 0)return true;
        addData(QByteArrayView(buffer,qsizetype(count)));
    }
}
QByteArray RHashSha256::result()
{
    if(impl->failed)throw std::runtime_error("SHA-256 stream has failed");
    if(!impl->digest.isEmpty())return impl->digest;
    try { impl->digest=impl->engine.finish(); } catch (...) { impl->failed=true;throw; }
    return impl->digest;
}
QByteArray RHashSha256::hash(QByteArrayView bytes)
{
#if defined(Q_OS_WIN) && !defined(RANGE_SHA256_TEST_QT_BACKEND)
    struct Local { std::unique_ptr<Engine> engine;bool busy=false; };
    thread_local Local local;
    if(local.busy)throw std::logic_error("Reentrant SHA-256 one-shot operation");
    local.busy=true;
    try {
        if(!local.engine)local.engine=std::make_unique<Engine>(true);
        local.engine->update(bytes);
        auto digest=local.engine->finish();
        local.busy=false;
        return digest;
    } catch (...) { local.engine.reset();local.busy=false;throw; }
#else
    RHashSha256 stream;stream.addData(bytes);return stream.result();
#endif
}
