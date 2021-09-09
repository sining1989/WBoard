#ifndef WBPageSizeUndoCommand_H_
#define WBPageSizeUndoCommand_H_

#include <QtWidgets>
#include "WBUndoCommand.h"

class WBGraphicsScene;

class WBPageSizeUndoCommand : public WBUndoCommand
{
    public:
        WBPageSizeUndoCommand(WBGraphicsScene* pScene, const QSize& previousSize, const QSize& newSize);
        virtual ~WBPageSizeUndoCommand();

        virtual int getType() { return WBUndoType::undotype_PAGESIZE; };

    protected:
        virtual void undo();
        virtual void redo();

    private:
        WBGraphicsScene* mScene;
        QSize mPreviousSize;
        QSize mNewSize;

        bool mFirstRedo;
};

#endif /* WBPageSizeUndoCommand_H_ */
