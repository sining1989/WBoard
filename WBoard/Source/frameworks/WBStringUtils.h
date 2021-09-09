#ifndef WBSTRINGUTILS_H
#define WBSTRINGUTILS_H

#include <QtCore>

class WBStringUtils
{
private:
    WBStringUtils() {}
    ~WBStringUtils() {}

public:
    static QStringList sortByLastDigit(const QStringList& source);

    static QString netxDigitizedName(const QString& source);

    static QString toCanonicalUuid(const QUuid& uuid);

    static QString toUtcIsoDateTime(const QDateTime& dateTime);
    static QDateTime fromUtcIsoDate(const QString& dateString);

};

#endif // WBSTRINGUTILS_H
