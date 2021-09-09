#include <QDebug>
#include <QNetworkProxy>
#include <QNetworkDiskCache>

#include "core/WBSettings.h"

#include "WBDownloadThread.h"

#include "core/memcheck.h"

WBDownloadThread::WBDownloadThread(QObject *parent, const char *name):QThread(parent)
    , mbRun(false)
    ,mpReply(NULL)
{
    setObjectName(name);
}

WBDownloadThread::~WBDownloadThread()
{
    if(NULL != mpReply)
    {
        delete mpReply;
        mpReply = NULL;
    }
}

void WBDownloadThread::run()
{
    qDebug() << mUrl;
    // We start the download
    QNetworkAccessManager* pNam = new QNetworkAccessManager();

    mpReply = pNam->get(QNetworkRequest(QUrl(mUrl)));
    qDebug() << " -- Http GET reply ---------------------- ";
    qDebug() << mpReply->readAll();
    qDebug() << " ---------------------------------------- ";

    connect(mpReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(onDownloadProgress(qint64,qint64)));
    connect(mpReply, SIGNAL(finished()), this, SLOT(onDownloadFinished()));

    while(mbRun)
    {
        // Wait here until the end of the download
        sleep(100);
    }

    disconnect(mpReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(onDownloadProgress(qint64,qint64)));
    disconnect(mpReply, SIGNAL(finished()), this, SLOT(onDownloadFinished()));
    if(NULL != mpReply)
    {
        delete mpReply;
        mpReply = NULL;
    }
}

void WBDownloadThread::stopDownload()
{
    mbRun = false;
}

void WBDownloadThread::startDownload(int id, QString url)
{
    mID = id;
    mUrl = url;
    mbRun = true;
    start();
}

void WBDownloadThread::onDownloadProgress(qint64 received, qint64 total)
{
    qDebug() << received << " on " << total;
    emit downloadProgress(mID, received, total);
}

void WBDownloadThread::onDownloadFinished()
{
    emit downloadFinised(mID);
}
