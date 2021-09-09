#include "WBPersistenceWorker.h"
#include "adaptors/WBSvgSubsetAdaptor.h"
#include "adaptors/WBThumbnailAdaptor.h"
#include "adaptors/WBMetadataDcSubsetAdaptor.h"

WBPersistenceWorker::WBPersistenceWorker(QObject *parent) :
    QObject(parent)
  , mReceivedApplicationClosing(false)
{
}

void WBPersistenceWorker::saveScene(WBDocumentProxy* proxy, WBGraphicsScene* scene, const int pageIndex)
{
    PersistenceInformation entry = {WriteScene, proxy, scene, pageIndex};

    saves.append(entry);
    mSemaphore.release();
}

void WBPersistenceWorker::readScene(WBDocumentProxy* proxy, const int pageIndex)
{
    PersistenceInformation entry = {ReadScene, proxy, 0, pageIndex};

    saves.append(entry);
    mSemaphore.release();
}

void WBPersistenceWorker::saveMetadata(WBDocumentProxy *proxy)
{
    PersistenceInformation entry = {WriteMetadata, proxy, NULL, 0};
    saves.append(entry);
    mSemaphore.release();
}

void WBPersistenceWorker::applicationWillClose()
{
    qDebug() << "applicaiton Will close signal received";
    mReceivedApplicationClosing = true;
    mSemaphore.release();
}

void WBPersistenceWorker::process()
{
    qDebug() << "process starts";
    mSemaphore.acquire();
    do{
        PersistenceInformation info = saves.takeFirst();
        if(info.action == WriteScene){
            WBSvgSubsetAdaptor::persistScene(info.proxy, info.scene, info.sceneIndex);
            emit scenePersisted(info.scene);
        }
        else if (info.action == ReadScene){
            emit sceneLoaded(WBSvgSubsetAdaptor::loadSceneAsText(info.proxy,info.sceneIndex), info.proxy, info.sceneIndex);
        }
        else if (info.action == WriteMetadata) {
            if (info.proxy->isModified()) {
                WBMetadataDcSubsetAdaptor::persist(info.proxy);
                emit metadataPersisted(info.proxy);
            }
        }
        mSemaphore.acquire();
    }while(!mReceivedApplicationClosing);
    qDebug() << "process will stop";
    emit finished();
}
