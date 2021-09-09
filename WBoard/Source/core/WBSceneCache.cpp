#include "WBSceneCache.h"

#include "domain/WBGraphicsScene.h"

#include "core/WBPersistenceManager.h"
#include "core/WBApplication.h"
#include "core/WBSettings.h"
#include "core/WBSetting.h"

#include "document/WBDocumentProxy.h"

#include "core/memcheck.h"

WBSceneCache::WBSceneCache()
    : mCachedSceneCount(0)
{
    // NOOP
}


WBSceneCache::~WBSceneCache()
{
    // NOOP
}


WBGraphicsScene* WBSceneCache::createScene(WBDocumentProxy* proxy, int pageIndex, bool useUndoRedoStack)
{
    WBGraphicsScene* newScene = new WBGraphicsScene(proxy, useUndoRedoStack);
    insert(proxy, pageIndex, newScene);

    return newScene;

}


void WBSceneCache::insert (WBDocumentProxy* proxy, int pageIndex, WBGraphicsScene* scene)
{
    QList<WBSceneCacheID> existingKeys = QHash<WBSceneCacheID, WBGraphicsScene*>::keys(scene);

    foreach(WBSceneCacheID key, existingKeys)
    {
        mCachedSceneCount -= QHash<WBSceneCacheID, WBGraphicsScene*>::remove(key);
    }

    WBSceneCacheID key(proxy, pageIndex);

    if (QHash<WBSceneCacheID, WBGraphicsScene*>::contains(key))
    {
        QHash<WBSceneCacheID, WBGraphicsScene*>::insert(key, scene);
        mCachedKeyFIFO.enqueue(key);
    }
    else
    {
        QHash<WBSceneCacheID, WBGraphicsScene*>::insert(key, scene);
        mCachedKeyFIFO.enqueue(key);

        mCachedSceneCount++;
    }

    if (mViewStates.contains(key))
    {
        scene->setViewState(mViewStates.value(key));
    }
}


bool WBSceneCache::contains(WBDocumentProxy* proxy, int pageIndex) const
{
    WBSceneCacheID key(proxy, pageIndex);
    return QHash<WBSceneCacheID, WBGraphicsScene*>::contains(key);
}


WBGraphicsScene* WBSceneCache::value(WBDocumentProxy* proxy, int pageIndex)
{
    WBSceneCacheID key(proxy, pageIndex);

    if (QHash<WBSceneCacheID, WBGraphicsScene*>::contains(key))
    {
        WBGraphicsScene* scene = QHash<WBSceneCacheID, WBGraphicsScene*>::value(key);

        mCachedKeyFIFO.removeAll(key);
        mCachedKeyFIFO.enqueue(key);

        return scene;
    }
    else
    {
        return 0;
    }
}


void WBSceneCache::removeScene(WBDocumentProxy* proxy, int pageIndex)
{
    WBGraphicsScene* scene = value(proxy, pageIndex);
    if (scene && !scene->isActive())
    {
        WBSceneCacheID key(proxy, pageIndex);
        int count = QHash<WBSceneCacheID, WBGraphicsScene*>::remove(key);
        mCachedKeyFIFO.removeAll(key);

        mViewStates.insert(key, scene->viewState());

        scene->deleteLater();

        mCachedSceneCount -= count;
    }
}


void WBSceneCache::removeAllScenes(WBDocumentProxy* proxy)
{
    for(int i = 0 ; i < proxy->pageCount(); i++)
    {
        removeScene(proxy, i);
    }
}


void WBSceneCache::moveScene(WBDocumentProxy* proxy, int sourceIndex, int targetIndex)
{
    WBSceneCacheID keySource(proxy, sourceIndex);

    WBGraphicsScene *scene = 0;

    if (QHash<WBSceneCacheID, WBGraphicsScene*>::contains(keySource))
    {
        scene = QHash<WBSceneCacheID, WBGraphicsScene*>::value(keySource);
        mCachedKeyFIFO.removeAll(keySource);
    }

    if (sourceIndex < targetIndex)
    {
        for (int i = sourceIndex + 1; i <= targetIndex; i++)
        {
            internalMoveScene(proxy, i, i - 1);
        }
    }
    else
    {
        for (int i = sourceIndex - 1; i >= targetIndex; i--)
        {
            internalMoveScene(proxy, i, i + 1);
        }
    }

    WBSceneCacheID keyTarget(proxy, targetIndex);

    if (scene)
    {
        insert(proxy, targetIndex, scene);
        mCachedKeyFIFO.enqueue(keyTarget);
    }
    else if (QHash<WBSceneCacheID, WBGraphicsScene*>::contains(keyTarget))
    {
        scene = QHash<WBSceneCacheID, WBGraphicsScene*>::take(keyTarget);
        mCachedKeyFIFO.removeAll(keyTarget);
    }

}

void WBSceneCache::reassignDocProxy(WBDocumentProxy *newDocument, WBDocumentProxy *oldDocument)
{
    if (!newDocument || !oldDocument) {
        return;
    }
    if (newDocument->pageCount() != oldDocument->pageCount()) {
        return;
    }
    if (!QFileInfo(oldDocument->persistencePath()).exists()) {
        return;
    }
    for (int i = 0; i < oldDocument->pageCount(); i++) {

        WBSceneCacheID sourceKey(oldDocument, i);
        WBGraphicsScene *currentScene = value(sourceKey);
        if (currentScene) {
            currentScene->setDocument(newDocument);
        }
        mCachedKeyFIFO.removeAll(sourceKey);
        int count = QHash<WBSceneCacheID, WBGraphicsScene*>::remove(sourceKey);
        mCachedSceneCount -= count;

        insert(newDocument, i, currentScene);
    }

}


void WBSceneCache::shiftUpScenes(WBDocumentProxy* proxy, int startIncIndex, int endIncIndex)
{
    for(int i = endIncIndex; i >= startIncIndex; i--)
    {
        internalMoveScene(proxy, i, i + 1);
    }
}


void WBSceneCache::internalMoveScene(WBDocumentProxy* proxy, int sourceIndex, int targetIndex)
{
    WBSceneCacheID sourceKey(proxy, sourceIndex);

    if (QHash<WBSceneCacheID, WBGraphicsScene*>::contains(sourceKey))
    {
        WBGraphicsScene* scene = QHash<WBSceneCacheID, WBGraphicsScene*>::take(sourceKey);
        mCachedKeyFIFO.removeAll(sourceKey);

        WBSceneCacheID targetKey(proxy, targetIndex);
        QHash<WBSceneCacheID, WBGraphicsScene*>::insert(targetKey, scene);
        mCachedKeyFIFO.enqueue(targetKey);

    }
    else
    {
        WBSceneCacheID targetKey(proxy, targetIndex);
        if (QHash<WBSceneCacheID, WBGraphicsScene*>::contains(targetKey))
        {
            /*WBGraphicsScene* scene = */QHash<WBSceneCacheID, WBGraphicsScene*>::take(targetKey);

            mCachedKeyFIFO.removeAll(targetKey);
        }
    }
}

void WBSceneCache::dumpCacheContent()
{
    foreach(WBSceneCacheID key, keys())
    {
        WBGraphicsScene *scene = 0;

        if (QHash<WBSceneCacheID, WBGraphicsScene*>::contains(key))
            scene = QHash<WBSceneCacheID, WBGraphicsScene*>::value(key);

        int index = key.pageIndex;

        qDebug() << "WBSceneCache::dumpCacheContent:" << index << " : " << scene;
    }
}
