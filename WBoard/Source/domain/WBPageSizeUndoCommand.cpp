#include "WBPageSizeUndoCommand.h"

#include <QtWidgets>

#include "core/WBApplication.h"
#include "board/WBBoardController.h"
#include "WBGraphicsScene.h"

#include "core/memcheck.h"

WBPageSizeUndoCommand::WBPageSizeUndoCommand(WBGraphicsScene* pScene, const QSize& previousSize, const QSize& newSize)
    : mScene(pScene)
        , mPreviousSize(previousSize)
        , mNewSize(newSize)
{
    mFirstRedo = true;
}

WBPageSizeUndoCommand::~WBPageSizeUndoCommand()
{
   //NOOP
}

void WBPageSizeUndoCommand::undo()
{
    if (!mScene){
        return;
    }

    WBApplication::boardController->setPageSize(mPreviousSize);
    // force refresh, QT is a bit lazy and take a lot of time (nb item ^2 ?) to trigger repaint
    mScene->update(mScene->sceneRect());

}

void WBPageSizeUndoCommand::redo()
{
    // the Undo framework calls a redo while appending the undo command.
    // as we have already plotted the elements, we do not want to do it twice
    if (!mFirstRedo)
    {
        if (!mScene){
            return;
        }

        WBApplication::boardController->setPageSize(mNewSize);
        // force refresh, QT is a bit lazy and take a lot of time (nb item ^2) to trigger repaint
        mScene->update(mScene->sceneRect());
    }
    else
    {
        mFirstRedo = false;
    }
}
