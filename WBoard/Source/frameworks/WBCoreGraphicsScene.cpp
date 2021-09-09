#include "WBCoreGraphicsScene.h"

#include "domain/WBGraphicsMediaItem.h"
#include "domain/WBGraphicsGroupContainerItem.h"

#include "core/memcheck.h"

WBCoreGraphicsScene::WBCoreGraphicsScene(QObject * parent)
    : QGraphicsScene ( parent  )
    , mIsModified(true)
{
    //NOOP
}

WBCoreGraphicsScene::~WBCoreGraphicsScene()
{
    //we must delete removed items that are no more in any scene
    //at groups deleting some items can be added to mItemsToDelete, so we need to use iterators.
    foreach(QGraphicsItem* item, mItemsToDelete)
    {
        if (item)
        {
            if (item->scene() == NULL || item->scene() == this)
            {
                delete item;
                item = NULL;
            }
        }
    }
    mItemsToDelete.clear();
}

void WBCoreGraphicsScene::addItem(QGraphicsItem* item)
{
    addItemToDeletion(item);

    if (item->type() == WBGraphicsGroupContainerItem::Type && item->childItems().count()) {
        foreach (QGraphicsItem *curItem, item->childItems()) {
            removeItemFromDeletion(curItem);
        }
    }

    if (item->scene() != this)
        QGraphicsScene::addItem(item);

    setModified(true);
}


void WBCoreGraphicsScene::removeItem(QGraphicsItem* item, bool forceDelete)
{
    QGraphicsScene::removeItem(item);
    if (forceDelete)
    {
        deleteItem(item);
    }
    setModified(true);
}

bool WBCoreGraphicsScene::deleteItem(QGraphicsItem* item)
{
    if(mItemsToDelete.contains(item))
    {
        WBGraphicsItem *item_casted = dynamic_cast<WBGraphicsItem *>(item);
        if (item_casted != NULL)
            item_casted->clearSource();

        mItemsToDelete.remove(item);
        delete item;
        item = NULL;
        return true;
    }
    else
        return false;
}

void WBCoreGraphicsScene::removeItemFromDeletion(QGraphicsItem *item)
{
    if(NULL != item){
        mItemsToDelete.remove(item);
    }
}

void WBCoreGraphicsScene::addItemToDeletion(QGraphicsItem *item)
{
    if (item) {
        mItemsToDelete.insert(item);
    }
}
