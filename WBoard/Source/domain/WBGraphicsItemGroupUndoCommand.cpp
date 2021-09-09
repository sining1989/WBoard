#include "WBGraphicsItemGroupUndoCommand.h"

#include "WBGraphicsGroupContainerItem.h"
#include "WBGraphicsScene.h"
#include "core/memcheck.h"


WBGraphicsItemGroupUndoCommand::WBGraphicsItemGroupUndoCommand(WBGraphicsScene *pScene, WBGraphicsGroupContainerItem *pGroupCreated) : WBUndoCommand()
  , mScene (pScene)
  , mGroup(pGroupCreated)
  , mFirstRedo(true)
{
    if (pGroupCreated->childItems().count()) {
        foreach (QGraphicsItem *item, pGroupCreated->childItems()) {
            mItems << item;
        }
    }
}

WBGraphicsItemGroupUndoCommand::~WBGraphicsItemGroupUndoCommand()
{
}

void WBGraphicsItemGroupUndoCommand::undo()
{
    mGroup->destroy(false);
    foreach(QGraphicsItem *item, mItems) {
        item->setSelected(true);
    }
}

void WBGraphicsItemGroupUndoCommand::redo()
{
    if (mFirstRedo) {
        //Work around. TODO determine why does Qt call the redo function on pushing to undo
        mFirstRedo = false;
        return;
    }

    foreach (QGraphicsItem *item, mItems) {
        if (item->type() == WBGraphicsGroupContainerItem::Type) {
            QList<QGraphicsItem*> childItems = item->childItems();
            WBGraphicsGroupContainerItem *currentGroup = dynamic_cast<WBGraphicsGroupContainerItem*>(item);
            if (currentGroup) {
                currentGroup->destroy(false);
            }
            foreach (QGraphicsItem *chItem, childItems) {
                mGroup->addToGroup(chItem);
            }
        } else {
            mGroup->addToGroup(item);
        }
    }

    mScene->addItem(mGroup);
    mGroup->setVisible(true);
    mGroup->setFocus();
    mGroup->setSelected(true);
}
