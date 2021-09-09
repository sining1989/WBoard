#ifndef WBBASE32_H_
#define WBBASE32_H_

#include <QtCore>

class WBBase32
{
    public:
        static QByteArray decode(const QString& base32String);

    protected:
        WBBase32() {}
        virtual ~WBBase32() {}

    private:
        static QString sBase32Chars;
        static int sBase32Lookup[];
};

#endif /* WBBASE32_H_ */
