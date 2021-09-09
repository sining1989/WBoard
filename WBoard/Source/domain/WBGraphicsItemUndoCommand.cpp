#include "WBGraphicsItemUndoCommand.h"

#include <QtWidgets>

#include "WBGraphicsScene.h"

#include "core/WBApplication.h"

#include "board/WBBoardController.h"

#include "core/memcheck.h"
#include "domain/WBGraphicsGroupContainerItem.h"
#include "domain/WBGraphicsPolygonItem.h"

WBGraphicsItemUndoCommand::WBGraphicsItemUndoCommand(WBGraphicsScene* pScene, const QSet<QGraphicsItem*>& pRemovedItems, const QSet<QGraphicsItem*>& pAddedItems, const GroupDataTable &groupsMap): WBUndoCommand()
    , mScene(pScene)
    , mRemovedItems(pRemovedItems - pAddedItems)
    , mAddedItems(pAddedItems - pRemovedItems)
    , mExcludedFromGroup(groupsMap)
{
    mFirstRedo = true;

    QSetIterator<QGraphicsItem*> itAdded(mAddedItems);
    while (itAdded.hasNext())
    {
        WBApplication::boardController->freezeW3CWidget(itAdded.next(), true);
    }

    QSetIterator<QGraphicsItem*> itRemoved(mRemovedItems);
    while (itRemoved.hasNext())
    {
        WBApplication::boardController->freezeW3CWidget(itRemoved.next(), false);
    }
}

WBGraphicsItemUndoCommand::WBGraphicsItemUndoCommand(WBGraphicsScene* pScene, QGraphicsItem* pRemovedItem, QGraphicsItem* pAddedItem) : WBUndoCommand()
    , mScene(pScene)
{

    if (pRemovedItem)
    {
        mRemovedItems.insert(pRemovedItem);
    }

    if (pAddedItem)
    {
        mAddedItems.insert(pAddedItem);
    }

    mFirstRedo = true;

}

WBGraphicsItemUndoCommand::~WBGraphicsItemUndoCommand()
{
   //NOOP
}

void WBGraphicsItemUndoCommand::undo()
{
    if (!mScene){
        return;
    }

    QSetIterator<QGraphicsItem*> itAdded(mAddedItems);
    while (itAdded.hasNext())
    {
        QGraphicsItem* item = itAdded.next();

        WBApplication::boardController->freezeW3CWidget(item, true);
        item->setSelected(false);

        QTransform t;
        bool bApplyTransform = false;
        WBGraphicsPolygonItem *polygonItem = qgraphicsitem_cast<WBGraphicsPolygonItem*>(item);
        if (polygonItem){
            if (polygonItem->strokesGroup()
                    && polygonItem->strokesGroup()->parentItem()
                    && WBGraphicsGroupContainerItem::Type == polygonItem->strokesGroup()->parentItem()->type())
            {
                bApplyTransform = true;
                t = polygonItem->sceneTransform();
            }
            else if (polygonItem->strokesGroup())
                polygonItem->resetTransform();

            polygonItem->strokesGroup()->removeFromGroup(polygonItem);
        }
        mScene->removeItem(item);

        if (bApplyTransform)
            polygonItem->setTransform(t);

    }

    QSetIterator<QGraphicsItem*> itRemoved(mRemovedItems);
    while (itRemoved.hasNext())
    {
        QGraphicsItem* item = itRemoved.next();
        if (item)
        {
            if (WBItemLayerType::FixedBackground == item->data(WBGraphicsItemData::ItemLayerType))
                mScene->setAsBackgroundObject(item);
            else
                mScene->addItem(item);

            if (WBGraphicsPolygonItem::Type == item->type())
            {
                WBGraphicsPolygonItem *polygonItem = qgraphicsitem_cast<WBGraphicsPolygonItem*>(item);
                if (polygonItem)
                {
                    mScene->removeItem(polygonItem);
                    mScene->removeItemFromDeletion(polygonItem);
                    polygonItem->strokesGroup()->addToGroup(polygonItem);
                }
            }

            WBApplication::boardController->freezeW3CWidget(item, false);
        }
    }

    QMapIterator<WBGraphicsGroupContainerItem*, QUuid> curMapElement(mExcludedFromGroup);
    WBGraphicsGroupContainerItem *nextGroup = NULL;
    WBGraphicsGroupContainerItem *previousGroupItem = NULL;
    bool groupChanged = false;

    while (curMapElement.hasNext()) {
        curMapElement.next();

        groupChanged = previousGroupItem != curMapElement.key();
        //trying to find the group on the scene;
        if (!nextGroup || groupChanged) {
            WBGraphicsGroupContainerItem *groupCandidate = curMapElement.key();
            if (groupCandidate) {
                nextGroup = groupCandidate;
                if(!mScene->items().contains(nextGroup)) {
                    mScene->addItem(nextGroup);
                }
                nextGroup->setVisible(true);
            }
        }

        QGraphicsItem *groupedItem = mScene->itemForUuid(curMapElement.value());
        if (groupedItem) {
            nextGroup->addToGroup(groupedItem);
        }

        previousGroupItem = curMapElement.key();
        WBGraphicsItem::Delegate(nextGroup)->update();
    }

    // force refresh, QT is a bit lazy and take a lot of time (nb item ^2 ?) to trigger repaint
    mScene->update(mScene->sceneRect());
    mScene->updateSelectionFrame();

}

void WBGraphicsItemUndoCommand::redo()
{
    // the Undo framework calls a redo while appending the undo command.
    // as we have already plotted the elements, we do not want to do it twice
    if (!mFirstRedo)
    {
        if (!mScene){
            return;
        }

        QMapIterator<WBGraphicsGroupContainerItem*, QUuid> curMapElement(mExcludedFromGroup);
        WBGraphicsGroupContainerItem *nextGroup = NULL;
        WBGraphicsGroupContainerItem *previousGroupItem = NULL;
        bool groupChanged = false;

        while (curMapElement.hasNext()) {
            curMapElement.next();

            groupChanged = previousGroupItem != curMapElement.key();
            //trying to find the group on the scene;
            if (!nextGroup || groupChanged) {
                WBGraphicsGroupContainerItem *groupCandidate = curMapElement.key();
                if (groupCandidate) {
                    nextGroup = groupCandidate;
                }
            }
            QGraphicsItem *groupedItem = mScene->itemForUuid(curMapElement.value());
            if (groupedItem) {
                if (nextGroup->childItems().count() == 1) {
                    nextGroup->destroy(false);
                    break;
                }
                nextGroup->removeFromGroup(groupedItem);
            }

            previousGroupItem = curMapElement.key();
            WBGraphicsItem::Delegate(nextGroup)->update();
        }

        QSetIterator<QGraphicsItem*> itRemoved(mRemovedItems);
        while (itRemoved.hasNext())
        {
            QGraphicsItem* item = itRemoved.next();
            item->setSelected(false);

            QTransform t;
            bool bApplyTransform = false;
            WBGraphicsPolygonItem *polygonItem = qgraphicsitem_cast<WBGraphicsPolygonItem*>(item);

            if (polygonItem){
                if(polygonItem->strokesGroup()
                        && polygonItem->strokesGroup()->parentItem()
                        && WBGraphicsGroupContainerItem::Type == polygonItem->strokesGroup()->parentItem()->type())
                {
                    bApplyTransform = true;
                    t = polygonItem->sceneTransform();
                }
                else if (polygonItem->strokesGroup())
                    polygonItem->resetTransform();

                polygonItem->strokesGroup()->removeFromGroup(polygonItem);
            }
            mScene->removeItem(item);

            if (bApplyTransform)
                item->setTransform(t);

            WBApplication::boardController->freezeW3CWidget(item, true);
        }

        QSetIterator<QGraphicsItem*> itAdded(mAddedItems);
        while (itAdded.hasNext())
        {
            QGraphicsItem* item = itAdded.next();
            if (item)
            {
                WBApplication::boardController->freezeW3CWidget(item, false);

                if (WBItemLayerType::FixedBackground == item->data(WBGraphicsItemData::ItemLayerType))
                    mScene->setAsBackgroundObject(item);
                else
                    mScene->addItem(item);

                WBGraphicsPolygonItem *polygonItem = qgraphicsitem_cast<WBGraphicsPolygonItem*>(item);
                if (polygonItem)
                {
                    mScene->removeItem(polygonItem);
                    mScene->removeItemFromDeletion(polygonItem);
                    polygonItem->strokesGroup()->addToGroup(polygonItem);
                }
            }
        }

        // force refresh, QT is a bit lazy and take a lot of time (nb item ^2) to trigger repaint
        mScene->update(mScene->sceneRect());
    }
    else
    {
        mFirstRedo = false;
    }
}
