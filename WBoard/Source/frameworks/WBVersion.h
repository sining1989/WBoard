#ifndef WBVERSION_H_
#define WBVERSION_H_

#include <QString>

enum ReleaseStage { Alpha = 10, Beta = 11, ReleaseCandidate = 15, Release = 17 };

class WBVersion
{
public:
    WBVersion(const QString &string);
    virtual ~WBVersion();

    uint toUInt() const;

    bool operator < (const WBVersion &otherVersion) const;
    bool operator == (const WBVersion &otherVersion) const;
    bool operator > (const WBVersion &otherVersion) const;

private:
    QString mString;
};

#endif // WBVERSION_H_
