#include "WBDownloadManager.h"
#include "core/WBApplication.h"
#include "core/WBPersistenceManager.h"
#include "gui/WBMainWindow.h"
#include "board/WBBoardController.h"
#include "board/WBBoardPaletteManager.h"
#include "frameworks/WBFileSystemUtils.h"

#include "core/memcheck.h"


WBAsyncLocalFileDownloader::WBAsyncLocalFileDownloader(sDownloadFileDesc desc, QObject *parent)
: QThread(parent)
, mDesc(desc)
, m_bAborting(false)
{

}

WBAsyncLocalFileDownloader *WBAsyncLocalFileDownloader::download()
{
    start();
    return this;
}

void WBAsyncLocalFileDownloader::run()
{

    if(mDesc.srcUrl.startsWith("file://"))
        mDesc.srcUrl = QUrl(mDesc.srcUrl).toLocalFile();
    else
        mDesc.srcUrl = QUrl::fromLocalFile(mDesc.srcUrl).toLocalFile();

    QString mimeType = WBFileSystemUtils::mimeTypeFromFileName(mDesc.srcUrl);

    int position=mimeType.indexOf(";");
    if(position != -1)
        mimeType=mimeType.left(position);

    WBMimeType::Enum itemMimeType = WBFileSystemUtils::mimeTypeFromString(mimeType);


    QString destDirectory;
    if (WBMimeType::Video == itemMimeType)
        destDirectory = WBPersistenceManager::videoDirectory;
    else 
        if (WBMimeType::Audio == itemMimeType)
            destDirectory = WBPersistenceManager::audioDirectory;

    if (mDesc.originalSrcUrl.isEmpty())
        mDesc.originalSrcUrl = mDesc.srcUrl;

    QString uuid = QUuid::createUuid().toString();
    WBPersistenceManager::persistenceManager()->addFileToDocument(WBApplication::boardController->selectedDocument(), 
        mDesc.srcUrl,
        destDirectory,
        uuid,
        mTo,
        NULL);

    if (m_bAborting)
    {
        if (QFile::exists(mTo))
            QFile::remove(mTo);
    }
    else
        emit signal_asyncCopyFinished(mDesc.id, !mTo.isEmpty(), QUrl::fromLocalFile(mTo), QUrl::fromLocalFile(mDesc.originalSrcUrl), "", NULL, mDesc.pos, mDesc.size, mDesc.isBackground);
}

void WBAsyncLocalFileDownloader::abort()
{
    m_bAborting = true;
}

/** The unique instance of the download manager */
static WBDownloadManager* pInstance = NULL;

/**
 * \brief Constructor
 * @param parent as the parent widget
 * @param name as the object name
 */
WBDownloadManager::WBDownloadManager(QObject *parent, const char *name):QObject(parent)
{
    setObjectName(name);
    init();

    connect(this, SIGNAL(fileAddedToDownload()), this, SLOT(onUpdateDownloadLists()));
}

/**
 * \brief Destructor
 */
WBDownloadManager::~WBDownloadManager()
{

}

/**
 * \brief Get the download manager
 * @return a pointer on the download manager
 */
WBDownloadManager* WBDownloadManager::downloadManager()
{
    if(NULL == pInstance)
    {
        pInstance = new WBDownloadManager();
    }
    return pInstance;
}

void WBDownloadManager::destroy()
{
    if(pInstance)
    {
        delete pInstance;
    }
    pInstance = NULL;
}

/**
 * \brief Add a file to the download list
 * @param desc as the given file description
 */
int WBDownloadManager::addFileToDownload(sDownloadFileDesc desc)
{
    // Set the ID for this download
    desc.id = mLastID;
    mLastID++;

    // Add the file to the pending download list
    mPendingDL.append(desc);

    // If the download is modal, show the download dialog
    if(desc.modal)
    {
        // Update the download order (priority to modal files)
        updateDownloadOrder();
        WBApplication::mainWindow->showDownloadWidget();
    }
    WBApplication::boardController->paletteManager()->startDownloads();

    emit fileAddedToDownload();

    return desc.id;
}

/**
 * \brief Initialize the download manager
 */
void WBDownloadManager::init()
{
    mCrntDL.clear();
    mPendingDL.clear();
    mDownloads.clear();
    mLastID = 1;
    mDLAvailability.clear();
    for(int i=0; i<SIMULTANEOUS_DOWNLOAD; i++)
    {
        mDLAvailability.append(-1);
    }
}

/**
 * \brief Update the download order. The modal downloads will be put in priority.
 */
void WBDownloadManager::updateDownloadOrder()
{
    QVector<sDownloadFileDesc> modalFiles;
    QVector<sDownloadFileDesc> nonModalfiles;

    for(int i=0; i<mPendingDL.size(); i++)
    {
        sDownloadFileDesc crnt = mPendingDL.at(i);
        if(crnt.modal)
        {
            modalFiles.append(crnt);
        }
        else
        {
            nonModalfiles.append(crnt);
        }
    }

    mPendingDL = modalFiles + nonModalfiles;
}

/**
 * \brief Update the download list. If a current download is finished, we take a
 *  file from the pending download list and add it to the download list.
 */
void WBDownloadManager::onUpdateDownloadLists()
{
    for(int i=0; i<SIMULTANEOUS_DOWNLOAD; i++)
    {
        if(mPendingDL.empty())
        {
            // If we fall here that means that there is no pending download
            break;
        }
        if(-1 == mDLAvailability.at(i))        {
            // Pending downloads exist and a download 'slot' is available
            // Let's move the first pending download to the current download
            // list and fill the slot
            sDownloadFileDesc desc = mPendingDL.at(0);
            mCrntDL.append(desc);
            mPendingDL.remove(0);
            mDLAvailability.remove(i);
            mDLAvailability.insert(i, desc.id);

            // Start the download of this file
            startFileDownload(desc);
        }
    }
}

/**
 * \brief Get the list of the current downloads
 * @return a QVector of current downloads
 */
QVector<sDownloadFileDesc> WBDownloadManager::currentDownloads()
{
    return mCrntDL;
}

/**
 * \brief Get the list of the pending downloads
 * @return a QVector of pending downloads
 */
QVector<sDownloadFileDesc> WBDownloadManager::pendingDownloads()
{
    return mPendingDL;
}

/**
 * \brief Update the file transfer information
 * @param desc as the current downloaded file description
 */
void WBDownloadManager::onDownloadProgress(int id, qint64 received, qint64 total)
{
    updateFileCurrentSize(id, received, total);
}

/**
 * \brief Called when the download of the given file is finished
 * @param desc as the current downloaded file description
 */

void WBDownloadManager::onDownloadFinished(int id, bool pSuccess, QUrl sourceUrl, QUrl contentUrl, QString pContentTypeHeader, QByteArray pData, QPointF pPos, QSize pSize, bool isBackground)
{
//    Temporary data for dnd do not delete it please
    Q_UNUSED(pPos)
    Q_UNUSED(pSize)
    Q_UNUSED(isBackground)

    for(int i=0; i<mCrntDL.size(); i++)
    {
        sDownloadFileDesc desc = mCrntDL.at(i);
        if(id == desc.id)
        {
            if (desc.dest == sDownloadFileDesc::graphicsWidget) {
                desc.contentTypeHeader = pContentTypeHeader;
                emit downloadFinished(pSuccess, desc, pData);

            } else if(desc.dest == sDownloadFileDesc::board) {
                // The downloaded file is modal so we must put it on the board
                emit addDownloadedFileToBoard(pSuccess, sourceUrl, contentUrl, pContentTypeHeader, pData, pPos, pSize, isBackground);
            }
            else
            {
                emit addDownloadedFileToLibrary(pSuccess, sourceUrl, pContentTypeHeader, pData, desc.name);
            }

            break;
        }
    }

    // Then do this
    updateFileCurrentSize(id);
}

/**
 * \brief Update the description of the given current downloaded file
 * @param desc as the current downloaded file description
 */
void WBDownloadManager::updateFileCurrentSize(int id, qint64 received, qint64 total)
{
    for(int i=0; i<mCrntDL.size();i++)
    {
        if(mCrntDL.at(i).id == id)
        {
            sDownloadFileDesc desc = mCrntDL.at(i);
            if(received >= 0 && total >= 0)
            {
                // -------------------------------------
                // [=============== x% ====            ]
                // -------------------------------------
                desc.currentSize = received;
                desc.totalSize = total;

                emit downloadUpdated(id, received, total);
            }
            else
            {
                // -------------------------------------
                // [=============== 100% ==============]
                // -------------------------------------
                // received and total are negative. That means that the download is finished
                desc.currentSize = mCrntDL.at(i).totalSize;

                // Remove the finished file from the current download list
                mCrntDL.remove(i);

                // Here we don't forget to remove the reply related to the finished download
                mDownloads.remove(id);

                // Free the download slot used by the finished file
                for(int j=0; j<mDLAvailability.size();j++)
                {
                    if(id == mDLAvailability.at(j))
                    {
                        mDLAvailability.remove(j);
                        mDLAvailability.insert(j, -1);
                        break;
                    }
                }

                // Here we check if some modal downloads remain
                checkIfModalRemains();

                // Then we update the list of downloads
                onUpdateDownloadLists();

                emit downloadFinished(id);

                // Verify if all downloads are finished
                if(mCrntDL.empty() && mPendingDL.empty())
                {
                    finishDownloads();
                }

                break;
            }

            mCrntDL.remove(i);
            mCrntDL.insert(i,desc);
            break;
        }
    }
}

/**
 * \brief Start the download of a file
 * @param desc as the given file description
 */
void WBDownloadManager::startFileDownload(sDownloadFileDesc desc)
{
    if (desc.srcUrl.startsWith("file://") || desc.srcUrl.startsWith("/"))
    {
        WBAsyncLocalFileDownloader * cpHelper = new WBAsyncLocalFileDownloader(desc, this);
        connect(cpHelper, SIGNAL(signal_asyncCopyFinished(int, bool, QUrl, QUrl, QString, QByteArray, QPointF, QSize, bool)), this, SLOT(onDownloadFinished(int, bool, QUrl, QUrl,QString, QByteArray, QPointF, QSize, bool)));
        QObject *res = dynamic_cast<QObject *>(cpHelper->download());
        if (!res)
            delete res;
        else
            mDownloads[desc.id] = res;
    }
    else
    {    
        WBDownloadHttpFile* http = new WBDownloadHttpFile(desc.id, this);
        connect(http, SIGNAL(downloadProgress(int, qint64,qint64)), this, SLOT(onDownloadProgress(int,qint64,qint64)));
        connect(http, SIGNAL(downloadFinished(int, bool, QUrl, QUrl, QString, QByteArray, QPointF, QSize, bool)), this, SLOT(onDownloadFinished(int, bool, QUrl, QUrl, QString, QByteArray, QPointF, QSize, bool)));
    
        //the desc.srcUrl is encoded. So we have to decode it before.
        QUrl url = QUrl::fromEncoded(desc.srcUrl.toUtf8());
        // We send here the request and store its reply in order to be able to cancel it if needed
        mDownloads[desc.id] = dynamic_cast<QObject *>(http->get(url, desc.pos, desc.size, desc.isBackground));
    } 
}

/**
 * \brief Verify if modal downloads remains and notify everyone if it is not the case.
 */
void WBDownloadManager::checkIfModalRemains()
{
    bool bModal = false;
    for(int i=0; i<mCrntDL.size();i++)
    {
        if(mCrntDL.at(i).modal)
        {
            bModal = true;
            break;
        }
    }

    if(!bModal)
    {
        for(int j=0; j<mPendingDL.size(); j++)
        {
            if(mPendingDL.at(j).modal)
            {
                bModal = true;
                break;
            }
        }
    }

    if(bModal || (mCrntDL.empty() && mPendingDL.empty()))
    {
        // Close the modal window
        WBApplication::mainWindow->hideDownloadWidget();

        // Notify that no modal downloads are pending
        emit downloadModalFinished();
    }
}

/**
 * \brief Cancel all downloads
 */
void WBDownloadManager::cancelDownloads()
{
    // Stop the current downloads
    QMap<int, QObject*>::iterator it = mDownloads.begin();
    for(; it!=mDownloads.end();it++)
    {
        QNetworkReply *netReply = dynamic_cast<QNetworkReply*>(it.value());
        if (netReply)
            netReply->abort();
        else
        {        
            WBAsyncLocalFileDownloader *localDownload = dynamic_cast<WBAsyncLocalFileDownloader *>(it.value());
            if (localDownload)
                localDownload->abort();
        }
    }

    // Clear all the lists
    init();

    checkIfModalRemains();

    finishDownloads(true);
}

void WBDownloadManager::onDownloadError(int id)
{
    QNetworkReply *pReply = dynamic_cast<QNetworkReply *>(mDownloads.value(id));
    
    if(NULL != pReply)
    {
        // Check which error occured:
        switch(pReply->error())
        {
            case QNetworkReply::OperationCanceledError:
                // For futur developments: do something in case of download aborting (message? remove the download?)
            break;

            default:
                // Check the documentation of QNetworkReply in Qt Assistant for the different error cases
            break;
        }
    }
}

void WBDownloadManager::finishDownloads(bool cancel)
{
    WBApplication::boardController->paletteManager()->stopDownloads();
    if(cancel){
        emit cancelAllDownloads();
    }
    else{
        emit allDownloadsFinished();
    }
}

void WBDownloadManager::cancelDownload(int id)
{
    if (!mDownloads.size())
        return;
   
    // Stop the download

    QNetworkReply *pNetworkDownload = dynamic_cast<QNetworkReply *>(mDownloads[id]);
    if (pNetworkDownload)
        pNetworkDownload->abort();
    else
    {
        WBAsyncLocalFileDownloader *pLocalDownload = dynamic_cast<WBAsyncLocalFileDownloader *>(mDownloads[id]);
        if (pLocalDownload)
        {
            if (pLocalDownload->isRunning())
                pLocalDownload->abort();                          
        }
    }

    mDownloads.remove(id);

    // Remove the canceled download from the download lists
    bool bFound = false;
    for(int i=0; i<mCrntDL.size(); i++){
        if(id == mCrntDL.at(i).id){
            mCrntDL.remove(i);
            bFound = true;
            break;
        }
    }
    if(!bFound){
        for(int j=0; j<mPendingDL.size(); j++){
            if(id == mPendingDL.at(j).id){
                mPendingDL.remove(j);
                bFound = true;
                break;
            }
        }
    }

    // Free the download slot used by the finished file
    for(int h=0; h<mDLAvailability.size();h++){
        if(id == mDLAvailability.at(h)){
            mDLAvailability.remove(h);
            mDLAvailability.insert(h, -1);
            break;
        }
    }

    // Here we check if some modal downloads remain
    checkIfModalRemains();

    // Then we update the list of downloads
    onUpdateDownloadLists();

    // Verify if all downloads are finished
    if(mCrntDL.empty() && mPendingDL.empty())
    {
        finishDownloads();
    }
}

// ------------------------------------------------------------------------------
/**
 * \brief Constructor
 * @param parent as the parent widget
 * @param name as the object name
 */
WBDownloadHttpFile::WBDownloadHttpFile(int fileId, QObject *parent):WBHttpGet(parent)
{
    mId = fileId;

    connect(this, SIGNAL(downloadFinished(bool,QUrl,QString,QByteArray,QPointF,QSize,bool)), this, SLOT(onDownloadFinished(bool,QUrl,QString,QByteArray,QPointF,QSize,bool)));
    connect(this, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(onDownloadProgress(qint64,qint64)));
}

/**
 * \brief Destructor
 */
WBDownloadHttpFile::~WBDownloadHttpFile()
{

}

/**
 * \brief Handles the download progress notification
 * @param bytesReceived as the number of received bytes
 * @param bytesTotal as the total number of bytes
 */
void WBDownloadHttpFile::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    emit downloadProgress(mId, bytesReceived, bytesTotal);
}

/**
 * \brief Handles the download finished notification
 * @param pSuccess as the success indicator
 * @param sourceUrl as the source URL
 * @param pContentTypeHeader as the response content type header
 * @param pData as the packet data
 * @param pPos as the item position in the board
 * @param psize as the item size (GUI)
 * @param isBackground as the background mdoe indicator
 */
void WBDownloadHttpFile::onDownloadFinished(bool pSuccess, QUrl sourceUrl, QString pContentTypeHeader, QByteArray pData, QPointF pPos, QSize pSize, bool isBackground)
{
    if(pSuccess)
    {
        // Notify the end of the download
        emit downloadFinished(mId, pSuccess, sourceUrl, sourceUrl, pContentTypeHeader, pData, pPos, pSize, isBackground);
    }
    else
    {
        // Notify the fact that and error occured during the download
        emit downloadError(mId);
    }
}

