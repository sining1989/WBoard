#include "WBGraphicsItemTransformUndoCommand.h"
#include "WBResizableGraphicsItem.h"
#include "domain/WBItem.h"
#include "domain/WBGraphicsScene.h"

#include "core/memcheck.h"

WBGraphicsItemTransformUndoCommand::WBGraphicsItemTransformUndoCommand(QGraphicsItem* pItem,
     const QPointF& prevPos, const QTransform& prevTransform, const qreal& prevZValue,
     const QSizeF& prevSize, bool setToBackground)
    : WBUndoCommand()
{
    mItem = pItem;
    mPreviousTransform = prevTransform;
    mCurrentTransform = pItem->transform();

    mPreviousPosition = prevPos;
    mCurrentPosition = pItem->pos();

    mPreviousZValue = prevZValue;
    mCurrentZValue = pItem->zValue();

    mPreviousSize = prevSize;
    WBResizableGraphicsItem* resizableItem = dynamic_cast<WBResizableGraphicsItem*>(pItem);

    if (resizableItem)
        mCurrentSize = resizableItem->size();

    mSetToBackground = setToBackground;
}

WBGraphicsItemTransformUndoCommand::~WBGraphicsItemTransformUndoCommand()
{
    // NOOP
}

void WBGraphicsItemTransformUndoCommand::undo()
{
    if (mSetToBackground) {
        WBGraphicsScene* scene = dynamic_cast<WBGraphicsScene*>(mItem->scene());
        if (scene && scene->isBackgroundObject(mItem)) {
            scene->unsetBackgroundObject();
        }
    }

    mItem->setPos(mPreviousPosition);
    mItem->setTransform(mPreviousTransform);
    mItem->setZValue(mPreviousZValue);

    WBResizableGraphicsItem* resizableItem = dynamic_cast<WBResizableGraphicsItem*>(mItem);

    if (resizableItem)
        resizableItem->resize(mPreviousSize);
}

void WBGraphicsItemTransformUndoCommand::redo()
{
    if (mSetToBackground) {
        WBGraphicsScene* scene = dynamic_cast<WBGraphicsScene*>(mItem->scene());
        if (scene)
            scene->setAsBackgroundObject(mItem);
    }

    mItem->setPos(mCurrentPosition);
    mItem->setTransform(mCurrentTransform);
    mItem->setZValue(mCurrentZValue);

    WBResizableGraphicsItem* resizableItem = dynamic_cast<WBResizableGraphicsItem*>(mItem);

    if (resizableItem)
        resizableItem->resize(mCurrentSize);
}
