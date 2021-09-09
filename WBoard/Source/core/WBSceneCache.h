#ifndef WBSCENECACHE_H
#define WBSCENECACHE_H

#include <QtCore>

#include "domain/WBGraphicsScene.h"

class WBDocumentProxy;
class WBGraphicsScene;
class WBGraphicsScene;

class WBSceneCacheID
{
public:
    WBSceneCacheID()
        : documentProxy(0)
        , pageIndex(-1)
    {

    }

    WBSceneCacheID(WBDocumentProxy* pDocumentProxy, int pPageIndex)
    {
        documentProxy = pDocumentProxy;
        pageIndex = pPageIndex;
    }

    WBDocumentProxy* documentProxy;
    int pageIndex;

};

inline bool operator==(const WBSceneCacheID &id1, const WBSceneCacheID &id2)
{
    return id1.documentProxy == id2.documentProxy
        && id1.pageIndex == id2.pageIndex;
}

inline uint qHash(const WBSceneCacheID &id)
{
    return qHash(id.pageIndex);
}

class WBSceneCache : public QHash<WBSceneCacheID, WBGraphicsScene*>
{
public:
    WBSceneCache();
    virtual ~WBSceneCache();

    WBGraphicsScene* createScene(WBDocumentProxy* proxy, int pageIndex, bool useUndoRedoStack);

    void insert (WBDocumentProxy* proxy, int pageIndex, WBGraphicsScene* scene );

    bool contains(WBDocumentProxy* proxy, int pageIndex) const;

    WBGraphicsScene* value(WBDocumentProxy* proxy, int pageIndex);

    WBGraphicsScene* value(const WBSceneCacheID& key) const
    {
        return QHash<WBSceneCacheID, WBGraphicsScene*>::value(key);
    }

    void removeScene(WBDocumentProxy* proxy, int pageIndex);

    void removeAllScenes(WBDocumentProxy* proxy);

    void moveScene(WBDocumentProxy* proxy, int sourceIndex, int targetIndex);

    void reassignDocProxy(WBDocumentProxy *newDocument, WBDocumentProxy *oldDocument);

    void shiftUpScenes(WBDocumentProxy* proxy, int startIncIndex, int endIncIndex);

private:
    void internalMoveScene(WBDocumentProxy* proxy, int sourceIndex, int targetIndex);

    void dumpCacheContent();

    int mCachedSceneCount;

    QQueue<WBSceneCacheID> mCachedKeyFIFO;

    QHash<WBSceneCacheID, WBGraphicsScene::SceneViewState> mViewStates;

};

#endif // WBSCENECACHE_H
