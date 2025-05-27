#ifndef CRYPTO_UTILS_H
#define CRYPTO_UTILS_H

#include <QString>
#include <QByteArray>
#include <openssl/sm2.h>
#include <openssl/sm3.h>
#include <openssl/sm4.h>

class CryptoUtils
{
public:
    static QByteArray generateSM2KeyPair(QByteArray &publicKey);
    static QByteArray sm3Hash(const QByteArray &data);
    static QByteArray sm4Encrypt(const QByteArray &data, const QByteArray &key);
    static QByteArray sm4Decrypt(const QByteArray &data, const QByteArray &key);
    static QByteArray sm2Encrypt(const QByteArray &data, const QByteArray &publicKey);
    static QByteArray sm2Decrypt(const QByteArray &data, const QByteArray &privateKey);
    static QByteArray sm2Sign(const QByteArray &data, const QByteArray &privateKey);
    static bool sm2Verify(const QByteArray &data, const QByteArray &signature, const QByteArray &publicKey);
    
    static QByteArray generateRandomBytes(int length);
    static QByteArray generateSalt();
    static QByteArray hashPassword(const QString &password, const QByteArray &salt);
};

#endif // CRYPTO_UTILS_H