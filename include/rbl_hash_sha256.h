#ifndef RBL_HASH_SHA256_H
#define RBL_HASH_SHA256_H

#include <QByteArray>
#include <QByteArrayView>
#include <memory>

class QIODevice;

//! SHA-256 of the exact supplied bytes. Instances belong to one thread.
class RHashSha256
{
public:
    RHashSha256();
    ~RHashSha256();
    RHashSha256(const RHashSha256 &) = delete;
    RHashSha256 &operator=(const RHashSha256 &) = delete;

    void addData(QByteArrayView bytes);
    //! Read from the device's current position; false on device admission/read failure.
    //! A failed read permanently invalidates this stream. Crypto failures throw.
    bool addData(QIODevice *device);
    //! Finalizes once; repeated calls return the same 32 raw bytes.
    //! Updates after finalization and all operations after failure throw.
    QByteArray result();
    static QByteArray hash(QByteArrayView bytes);

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

#endif
