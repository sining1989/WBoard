#ifndef WBCRYPTOUTILS_H_
#define WBCRYPTOUTILS_H_

#include <QtCore>
#include <openssl/evp.h>

#include "core/WBApplication.h"

class WBCryptoUtils : public QObject
{
    Q_OBJECT;

    public:
        static WBCryptoUtils* instance();
        static void destroy();

        QString symetricEncrypt(const QString& clear);
        QString symetricDecrypt(const QString& encrypted);

    private:
        WBCryptoUtils(QObject * pParent = 0);
        virtual ~WBCryptoUtils();

        static WBCryptoUtils* sInstance;
        static QString sAESKey;
        static QString sAESSalt;

        void aesInit();

#if OPENSSL_VERSION_NUMBER >= 10100000L
        EVP_CIPHER_CTX *mAesEncryptContext;
        EVP_CIPHER_CTX *mAesDecryptContext;
#else
        EVP_CIPHER_CTX mAesEncryptContext;
        EVP_CIPHER_CTX mAesDecryptContext;
#endif

};

#endif /* WBCRYPTOUTILS_H_ */
