#ifndef WBGRAPHICSITEMUNDOCOMMAND_H_
#define WBGRAPHICSITEMUNDOCOMMAND_H_

#include <QtWidgets>
#include "WBUndoCommand.h"
#include "WBGraphicsGroupContainerItem.h"

class WBGraphicsScene;
class WBGraphicsItemUndoCommand : public WBUndoCommand
{
    public:
        typedef QMultiMap<WBGraphicsGroupContainerItem*, QUuid> GroupDataTable;

        WBGraphicsItemUndoCommand(WBGraphicsScene* pScene, const QSet<QGraphicsItem*>& pRemovedItems,
                                  const QSet<QGraphicsItem*>& pAddedItems, const GroupDataTable &groupsMap = GroupDataTable());

        WBGraphicsItemUndoCommand(WBGraphicsScene* pScene, QGraphicsItem* pRemovedItem,
                        QGraphicsItem* pAddedItem);

        virtual ~WBGraphicsItemUndoCommand();

        QSet<QGraphicsItem*> GetAddedList() const { return mAddedItems; }
        QSet<QGraphicsItem*> GetRemovedList() const { return mRemovedItems; }

        virtual int getType() const { return WBUndoType::undotype_GRAPHICITEM; }

    protected:
        virtual void undo();
        virtual void redo();

    private:
        WBGraphicsScene* mScene;
        QSet<QGraphicsItem*> mRemovedItems;
        QSet<QGraphicsItem*> mAddedItems;
        GroupDataTable mExcludedFromGroup;

        bool mFirstRedo;
};

#endif /* WBGRAPHICSITEMUNDOCOMMAND_H_ */
