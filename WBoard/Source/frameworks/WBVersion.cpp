#include "WBVersion.h"

#include <QtCore>
#include <QStringList>

#include "core/memcheck.h"

WBVersion::WBVersion(const QString &string)
{
    mString = string;
}

uint WBVersion::toUInt() const
{
    uint result = 0;
    QStringList list = mString.split(QRegExp("[-\\.]"));
    switch (list.count()) {
    case 2:
        //short version  1.0
        result = (list.at(0).toUInt() * 100000000) + (list.at(1).toUInt() * 1000000) + (Release * 100);
        break;
    case 3:
        //release version 1.0.0
        result = (list.at(0).toUInt() * 100000000) + (list.at(1).toUInt() * 1000000) + list.at(2).toUInt()*10000 + (Release * 100);
        break;
    case 5:{
        //standard version  1.0.0.a/b/rc.0
        uint releaseStage = list.at(3).startsWith("a") ? Alpha :(list.at(3).startsWith("b") ? Beta : ReleaseCandidate);
        result = (list.at(0).toUInt() * 100000000) + (list.at(1).toUInt() * 1000000) + (list.at(2).toUInt() * 10000) + (releaseStage * 100) + list.at(4).toUInt();
        break;
    }
    default:
        qWarning() << "Unknown version format.";
        break;
    }
    return result;
}

WBVersion::~WBVersion()
{
    // NOOP
}

bool WBVersion::operator < (const WBVersion &otherVersion) const
{
    return toUInt() < otherVersion.toUInt();
}

bool WBVersion::operator == (const WBVersion &otherVersion) const
{
    return toUInt() == otherVersion.toUInt();
}

bool WBVersion::operator > (const WBVersion &otherVersion) const
{
    return toUInt() > otherVersion.toUInt();
}
