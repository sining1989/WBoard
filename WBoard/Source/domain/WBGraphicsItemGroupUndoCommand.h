#ifndef WBGRAPHICSITEMGROUPUNDOCOMMAND_H
#define WBGRAPHICSITEMGROUPUNDOCOMMAND_H

#include <QList>
#include "WBUndoCommand.h"

class WBGraphicsScene;
class WBGraphicsGroupContainerItem;

class WBGraphicsItemGroupUndoCommand : public WBUndoCommand
{
public:
    WBGraphicsItemGroupUndoCommand(WBGraphicsScene *pScene, WBGraphicsGroupContainerItem *pGroupCreated);
    virtual ~WBGraphicsItemGroupUndoCommand();

    virtual int getType() const { return WBUndoType::undotype_GRAPHICSGROUPITEM; }

protected:
    virtual void undo();
    virtual void redo();

private:
    WBGraphicsScene *mScene;
    WBGraphicsGroupContainerItem *mGroup;
    QList<QGraphicsItem*> mItems;

    bool mFirstRedo;
};

#endif // WBGRAPHICSITEMGROUPUNDOCOMMAND_H
