#include "WBHttpGet.h"

#include <QtNetwork>

#include "network/WBNetworkAccessManager.h"
#include "core/WBDownloadManager.h"

#include "core/memcheck.h"

sDownloadFileDesc desc;

WBHttpGet::WBHttpGet(QObject* parent)
    : QObject(parent)
    , mReply(0)
    , mIsBackground(false)
    , mRedirectionCount(0)
    , mIsSelfAborting(false)
{
    // NOOP
}


WBHttpGet::~WBHttpGet()
{
        if (mReply)
    {
        mIsSelfAborting = true;
        mReply->abort();
                delete mReply;
    }
}

QNetworkReply* WBHttpGet::get(QUrl pUrl, QPointF pPos, QSize pSize, bool isBackground)
{
    mPos = pPos;
    mSize = pSize;
    mIsBackground = isBackground;

    if (mReply)
        delete mReply;

    WBNetworkAccessManager * nam = WBNetworkAccessManager::defaultAccessManager();
    mReply = nam->get(QNetworkRequest(pUrl)); //mReply deleted by this destructor

    mDownloadedBytes.clear();

    connect(mReply, SIGNAL(finished()), this, SLOT(requestFinished()));
    connect(mReply, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(mReply, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(downloadProgressed(qint64, qint64)));

    return mReply;
}

void WBHttpGet::readyRead()
{
        if (mReply)
                mDownloadedBytes += mReply->readAll();
}


void WBHttpGet::requestFinished()
{
    if (!mReply || mIsSelfAborting)
    {
        return;
    }

    if (mReply->error() != QNetworkReply::NoError)
    {
        qWarning() << mReply->url().toString() << "get finished with error : " << mReply->error();

        mDownloadedBytes.clear();

        mRedirectionCount = 0;

        emit downloadFinished(false, mReply->url(), mReply->errorString(), mDownloadedBytes, mPos, mSize, mIsBackground);
    }
    else
    {

        qDebug() << mReply->url().toString() << "http get finished ...";

        if (mReply->header(QNetworkRequest::LocationHeader).isValid() && mRedirectionCount < 10)
        {
            mRedirectionCount++;
            get(mReply->header(QNetworkRequest::LocationHeader).toUrl(), mPos, mSize, mIsBackground);

            return;
        }

        mRedirectionCount = 0;

        emit downloadFinished(true, mReply->url(), mReply->header(QNetworkRequest::ContentTypeHeader).toString(),
                        mDownloadedBytes, mPos, mSize, mIsBackground);
    }

}

void WBHttpGet::downloadProgressed(qint64 bytesReceived, qint64 bytesTotal)
{
//    qDebug() << "received: " << bytesReceived << ", / " << bytesTotal << " bytes";
    if (-1 != bytesTotal)
    {
        emit downloadProgress(bytesReceived, bytesTotal);
    }
}

