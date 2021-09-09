#ifndef WBDOWNLOADMANAGER_H
#define WBDOWNLOADMANAGER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QMutex>
#include <QDropEvent>

#include "WBDownloadThread.h"

#include "network/WBHttpGet.h"

#define SIMULTANEOUS_DOWNLOAD 2 

struct sDownloadFileDesc
{
    enum eDestinations {
        board 
        , library
        , graphicsWidget
    };

    sDownloadFileDesc() :
        dest(board)
      , id(0)
      , totalSize(0)
      , currentSize(0)
      , modal(false)
      , isBackground(false)
      , dropActions(Qt::IgnoreAction)
      , dropMouseButtons(Qt::NoButton)
      , dropModifiers(Qt::NoModifier)
    {}

    eDestinations dest;
    QString name;
    int id;
    int totalSize;
    int currentSize;
    QString srcUrl;
    QString originalSrcUrl;
    QString contentTypeHeader;
    bool modal;
    QPointF pos; 
    QSize size;
    bool isBackground;

    QPoint dropPoint;
    Qt::DropActions dropActions;
    Qt::MouseButtons dropMouseButtons;
    Qt::KeyboardModifiers dropModifiers;
};

class WBDownloadHttpFile : public WBHttpGet
{
    Q_OBJECT
public:
    WBDownloadHttpFile(int fileId, QObject* parent=0);
    ~WBDownloadHttpFile();

signals:
    void downloadProgress(int id, qint64 current,qint64 total);
    void downloadFinished(int id, bool pSuccess, QUrl sourceUrl, QUrl contentUrl, QString pContentTypeHeader, QByteArray pData, QPointF pPos, QSize pSize, bool isBackground);
    void downloadError(int id);

private slots:
    void onDownloadFinished(bool pSuccess, QUrl sourceUrl, QString pContentTypeHeader, QByteArray pData, QPointF pPos, QSize pSize, bool isBackground);
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);

private:
    int mId;
};

class WBAsyncLocalFileDownloader : public QThread
{
    Q_OBJECT
public:
    WBAsyncLocalFileDownloader(sDownloadFileDesc desc, QObject *parent = 0);

    WBAsyncLocalFileDownloader *download();    
    void run();
    void abort();

signals:
    void finished(QString srcUrl, QString resUrl);
    void signal_asyncCopyFinished(int id, bool pSuccess, QUrl sourceUrl, QUrl contentUrl, QString pContentTypeHeader, QByteArray pData, QPointF pPos, QSize pSize, bool isBackground);

private:
    sDownloadFileDesc mDesc;
    bool m_bAborting;
    QString mFrom;
    QString mTo;
};

class WBDownloadManager : public QObject
{
    Q_OBJECT
public:
    WBDownloadManager(QObject* parent=0, const char* name="WBDownloadManager");
    ~WBDownloadManager();
    static WBDownloadManager* downloadManager();
    int addFileToDownload(sDownloadFileDesc desc);
    QVector<sDownloadFileDesc> currentDownloads();
    QVector<sDownloadFileDesc> pendingDownloads();
    void cancelDownloads();
    void cancelDownload(int id);

    static void destroy();

signals:
    void fileAddedToDownload();
    void downloadUpdated(int id, qint64 crnt, qint64 total);
    void downloadFinished(int id);
    void downloadFinished(bool pSuccess, int id, QUrl sourceUrl, QString pContentTypeHeader, QByteArray pData);
    void downloadFinished(bool pSuccess, sDownloadFileDesc desc, QByteArray pData);
    void downloadModalFinished();
    void addDownloadedFileToBoard(bool pSuccess, QUrl sourceUrl, QUrl contentUrl, QString pContentTypeHeader, QByteArray pData, QPointF pPos, QSize pSize, bool isBackground);
    void addDownloadedFileToLibrary(bool pSuccess, QUrl sourceUrl, QString pContentTypeHeader, QByteArray pData, QString pTitle);
    void cancelAllDownloads();
    void allDownloadsFinished();

private slots:
    void onUpdateDownloadLists();
    void onDownloadProgress(int id, qint64 received, qint64 total);
    void onDownloadFinished(int id, bool pSuccess, QUrl sourceUrl, QUrl contentUrl, QString pContentTypeHeader, QByteArray pData, QPointF pPos, QSize pSize, bool isBackground);
    void onDownloadError(int id);

private:
    void init();
    void updateDownloadOrder();
    void updateFileCurrentSize(int id, qint64 received=-1, qint64 total=-1);
    void startFileDownload(sDownloadFileDesc desc);
    void checkIfModalRemains();
    void finishDownloads(bool cancel=false);

    QVector<sDownloadFileDesc> mCrntDL;
    QVector<sDownloadFileDesc> mPendingDL;
    QMutex mMutex;
    int mLastID;
    QVector<int> mDLAvailability;
    QMap<int, QObject*> mDownloads;
};

#endif // WBDOWNLOADMANAGER_H
