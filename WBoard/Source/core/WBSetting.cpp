#include "WBSetting.h"

#include <QtWidgets>

#include "WBSettings.h"

#include "core/memcheck.h"

WBSetting::WBSetting(WBSettings* parent) :
    QObject(parent)
{
    //NOOP
}

WBSetting::WBSetting(WBSettings* owner, const QString& pDomain, const QString& pKey,
    const QVariant& pDefaultValue) :
	QObject(owner), 
    mOwner(owner), 
    mDomain(pDomain), 
    mKey(pKey), 
    mPath(pDomain + "/" + pKey), 
    mDefaultValue(pDefaultValue)
{
    get(); // force caching of the setting
}

WBSetting::~WBSetting()
{
    // NOOP
}

void WBSetting::set(const QVariant& pValue)
{
    if (pValue != get())
    {
        mOwner->setValue(mPath, pValue);
        emit changed(pValue);
    }
}

QVariant WBSetting::get()
{
    return mOwner->value(mPath, mDefaultValue);
}

QVariant WBSetting::reset()
{
    set(mDefaultValue);
    return mDefaultValue;
}


void WBSetting::setBool(bool pValue)
{
    set(pValue);
}

void WBSetting::setString(const QString& pValue)
{
    set(pValue);
}
void WBSetting::setInt(int pValue)
{
    set(pValue);
}


WBColorListSetting::WBColorListSetting(WBSettings* parent)
    : WBSetting(parent)
{
    //NOOP
}

WBColorListSetting::WBColorListSetting(WBSettings* owner, const QString& pDomain,
        const QString& pKey, const QVariant& pDefaultValue, qreal pAlpha)
    : WBSetting(owner, pDomain, pKey, pDefaultValue)
    , mAlpha(pAlpha)
{

    foreach(QString s, get().toStringList())
    {
        QColor color;
        color.setNamedColor(s);
        if (mAlpha>=0)
            color.setAlphaF(mAlpha);
        mColors.append(color);
    }

}

WBColorListSetting::~WBColorListSetting()
{
    // NOOP
}

QVariant WBColorListSetting::reset()
{
    QVariant result = WBSetting::reset();

    mColors.clear();

    foreach(QString s, get().toStringList())
    {
        QColor color;
        color.setNamedColor(s);
        if (mAlpha>=0)
            color.setAlphaF(mAlpha);

        mColors.append(color);
    }

    mOwner->colorChanged();

    return result;
}

QList<QColor> WBColorListSetting::colors() const
{
    return mColors;
}

void WBColorListSetting::setColor(int pIndex, const QColor& color)
{
    QStringList list = get().toStringList();
    list.replace(pIndex, color.name(QColor::HexArgb));
    if (mAlpha>=0)
    {
        QColor c = color;
        c.setAlphaF(mAlpha);
        mColors.replace(pIndex, c);
    }
    else
        mColors.replace(pIndex, color);
    set(list);
}

void WBColorListSetting::setAlpha(qreal pAlpha)
{
    mAlpha = pAlpha;

    for(int i = 0 ; i < mColors.size() ; i ++)
    {
        QColor c = mColors.at(i);
        c.setAlphaF(mAlpha);
        mColors.replace(i, c);
    }

    mOwner->colorChanged();
}
