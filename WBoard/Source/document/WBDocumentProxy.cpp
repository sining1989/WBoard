#include "WBDocumentProxy.h"

#include "frameworks/WBStringUtils.h"

#include "core/WBApplication.h"
#include "core/WBPersistenceManager.h"
#include "core/WBSettings.h"
#include "core/WBDocumentManager.h"
#include "core/memcheck.h"

#include "adaptors/WBMetadataDcSubsetAdaptor.h"

WBDocumentProxy::WBDocumentProxy()
    : mPageCount(0)
    , mPageDpi(0)
    , mPersistencePath("")
{
    init();
}

WBDocumentProxy::WBDocumentProxy(const WBDocumentProxy &rValue) :
    QObject()
{
    mPersistencePath = rValue.mPersistencePath;
    mMetaDatas = rValue.mMetaDatas;
    mIsModified = rValue.mIsModified;
    mPageCount = rValue.mPageCount;
}

WBDocumentProxy::WBDocumentProxy(const QString& pPersistancePath)
    : mPageCount(0)
    , mPageDpi(0)
{
    init();
    setPersistencePath(pPersistancePath);

    mMetaDatas = WBMetadataDcSubsetAdaptor::load(pPersistancePath);
}


WBDocumentProxy::WBDocumentProxy(const QString& pPersistancePath, QMap<QString, QVariant> metadatas)
    : mPageCount(0)
    , mPageDpi(0)
{
    init();
    setPersistencePath(pPersistancePath);

    mMetaDatas = metadatas;
}


void WBDocumentProxy::init()
{
    setMetaData(WBSettings::documentGroupName, "");

    QDateTime now = QDateTime::currentDateTime();
    setMetaData(WBSettings::documentName, now.toString(Qt::SystemLocaleShortDate));

    setUuid(QUuid::createUuid());

    setDefaultDocumentSize(WBSettings::settings()->pageSize->get().toSize());
}

bool WBDocumentProxy::theSameDocument(WBDocumentProxy *proxy)
{
    return  proxy && mPersistencePath == proxy->mPersistencePath;
}

WBDocumentProxy::~WBDocumentProxy()
{
    // NOOP
}

WBDocumentProxy* WBDocumentProxy::deepCopy() const
{
    WBDocumentProxy* copy = new WBDocumentProxy();

    copy->mPersistencePath = QString(mPersistencePath);
    copy->mMetaDatas = QMap<QString, QVariant>(mMetaDatas);
    copy->mIsModified = mIsModified;
    copy->mPageCount = mPageCount;

    return copy;
}


int WBDocumentProxy::pageCount()
{
    return mPageCount;
}


void WBDocumentProxy::setPageCount(int pPageCount)
{
    mPageCount = pPageCount;
}

int WBDocumentProxy::pageDpi()
{
    return mPageDpi;
}

void WBDocumentProxy::setPageDpi(int dpi)
{
    mPageDpi = dpi;
}

int WBDocumentProxy::incPageCount()
{
    if (mPageCount <= 0)
    {
        mPageCount = 1;
    }
    else
    {
        mPageCount++;
    }

    return mPageCount;

}


int WBDocumentProxy::decPageCount()
{
    mPageCount --;

    if (mPageCount < 0)
    {
        mPageCount = 0;
    }

    return mPageCount;
}

QString WBDocumentProxy::persistencePath() const
{
    return mPersistencePath;
}

void WBDocumentProxy::setPersistencePath(const QString& pPersistencePath)
{
    if (pPersistencePath != mPersistencePath)
    {
        mIsModified = true;
        mPersistencePath = pPersistencePath;
    }
}

void WBDocumentProxy::setMetaData(const QString& pKey, const QVariant& pValue)
{
    if (mMetaDatas.contains(pKey) && mMetaDatas.value(pKey) == pValue)
        return;
    else
    {
        mIsModified = true;
        mMetaDatas.insert(pKey, pValue);
        if (pKey == WBSettings::documentUpdatedAt)
        {
            WBDocumentManager *documentManager = WBDocumentManager::documentManager();
            if (documentManager)
                documentManager->emitDocumentUpdated(this);
        }
    }
}

QVariant WBDocumentProxy::metaData(const QString& pKey) const
{
    if (mMetaDatas.contains(pKey))
    {
        return mMetaDatas.value(pKey);
    }
    else
    {
        qDebug() << "Unknown metadata key" << pKey;
        return QString(""); // failsafe
    }
}

QMap<QString, QVariant> WBDocumentProxy::metaDatas() const
{
    return mMetaDatas;
}

QString WBDocumentProxy::name() const
{
    return metaData(WBSettings::documentName).toString();
}

QString WBDocumentProxy::groupName() const
{
    return metaData(WBSettings::documentGroupName).toString();
}

QSize WBDocumentProxy::defaultDocumentSize() const
{
    if (mMetaDatas.contains(WBSettings::documentSize))
        return metaData(WBSettings::documentSize).toSize();
    else
        return WBSettings::settings()->pageSize->get().toSize();
}

void WBDocumentProxy::setDefaultDocumentSize(QSize pSize)
{
    if (defaultDocumentSize() != pSize)
    {
        setMetaData(WBSettings::documentSize, QVariant(pSize));
        mIsModified = true;
    }
}

void WBDocumentProxy::setDefaultDocumentSize(int pWidth, int pHeight)
{
    setDefaultDocumentSize(QSize(pWidth, pHeight));
}


QUuid WBDocumentProxy::uuid() const
{
    QString id = metaData(WBSettings::documentIdentifer).toString();
    QString sUuid = id.replace(WBSettings::uniboardDocumentNamespaceUri + "/", "");

    return QUuid(sUuid);
}

void WBDocumentProxy::setUuid(const QUuid& uuid)
{
    setMetaData(WBSettings::documentIdentifer,
            WBSettings::uniboardDocumentNamespaceUri + "/" + WBStringUtils::toCanonicalUuid(uuid));
}


QDateTime WBDocumentProxy::documentDate()
{
    if(mMetaDatas.contains(WBSettings::documentDate))
        return WBStringUtils::fromUtcIsoDate(metaData(WBSettings::documentDate).toString());
    return QDateTime::currentDateTime();
}

QDateTime WBDocumentProxy::lastUpdate()
{
    if(mMetaDatas.contains(WBSettings::documentUpdatedAt))
        return WBStringUtils::fromUtcIsoDate(metaData(WBSettings::documentUpdatedAt).toString());
    return QDateTime::currentDateTime();
}

bool WBDocumentProxy::isModified() const
{
    return mIsModified;
}





