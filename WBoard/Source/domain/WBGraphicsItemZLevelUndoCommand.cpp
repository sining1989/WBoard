#include "WBGraphicsItemZLevelUndoCommand.h"

WBGraphicsItemZLevelUndoCommand::WBGraphicsItemZLevelUndoCommand(WBGraphicsScene *_scene, const QList<QGraphicsItem*>& _items, qreal _previousZLevel, WBZLayerController::moveDestination dest):WBUndoCommand(){
    Q_ASSERT(_scene != NULL);
    mpScene = _scene;
    mItems = _items;
    mPreviousZLevel = _previousZLevel;
    mDest = dest;
    mHack = false;
}

WBGraphicsItemZLevelUndoCommand::WBGraphicsItemZLevelUndoCommand(WBGraphicsScene *_scene, QGraphicsItem* _item, qreal _previousZLevel, WBZLayerController::moveDestination dest):WBUndoCommand(){
    Q_ASSERT(_scene != NULL);
    mpScene = _scene;
    if(NULL != _item)
        mItems.append(_item);

    mPreviousZLevel = _previousZLevel;
    mDest = dest;
    mHack = false;
}

WBGraphicsItemZLevelUndoCommand::~WBGraphicsItemZLevelUndoCommand(){

}

void WBGraphicsItemZLevelUndoCommand::undo(){
    if(!mpScene || mItems.empty())
        return;

    // Getting the difference between the initial z-value and the actual one
    qreal zDiff = qAbs(mItems.at(mItems.size()-1)->data(WBGraphicsItemData::ItemOwnZValue).toReal() - mPreviousZLevel);

    if(mDest == WBZLayerController::down || mDest == WBZLayerController::bottom){
        // Move up
        QList<QGraphicsItem*>::iterator downIt = mItems.end();
        for(; downIt >= mItems.begin(); downIt--){
            for(int i=0; i<zDiff; i++)
                mpScene->changeZLevelTo(*downIt, WBZLayerController::up);
        }

    }else if(mDest == WBZLayerController::up || mDest == WBZLayerController::top){
        // Move down
        foreach(QGraphicsItem* item, mItems){
            for(int i=0; i<zDiff; i++)
                mpScene->changeZLevelTo(item, WBZLayerController::down);
        }
    }

}

void WBGraphicsItemZLevelUndoCommand::redo(){
    if(!mHack){
        // Ugly! But pushing a new command to QUndoStack calls redo by itself.
        mHack = true;
    }else{
        if(!mpScene)
            return;

        foreach(QGraphicsItem* item, mItems){
            mpScene->changeZLevelTo(item, mDest);
            updateLazyScene();
        }
    }
}

void WBGraphicsItemZLevelUndoCommand::updateLazyScene(){
    mpScene->update(mpScene->sceneRect());
    mpScene->updateSelectionFrame();
}
