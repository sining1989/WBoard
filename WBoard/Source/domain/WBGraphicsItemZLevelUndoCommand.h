#ifndef WBGRAPHICSITEMZLEVELUNDOCOMMAND_H
#define WBGRAPHICSITEMZLEVELUNDOCOMMAND_H

#include <QGraphicsItem>

#include "WBUndoCommand.h"
#include "WBGraphicsScene.h"

class WBGraphicsItemZLevelUndoCommand : public WBUndoCommand
{
	public:
		WBGraphicsItemZLevelUndoCommand(WBGraphicsScene* _scene, const QList<QGraphicsItem*>& _items, qreal _previousZLevel, WBZLayerController::moveDestination dest);
		WBGraphicsItemZLevelUndoCommand(WBGraphicsScene* _scene, QGraphicsItem* _item, qreal _previousZLevel, WBZLayerController::moveDestination dest);
		~WBGraphicsItemZLevelUndoCommand();

		virtual int getType() const { return WBUndoType::undotype_GRAPHICITEMZVALUE; }

	protected:
		virtual void undo();
		virtual void redo();

	private:
		void updateLazyScene();

		qreal mPreviousZLevel;
		QList<QGraphicsItem*> mItems;
		WBGraphicsScene* mpScene;
		WBZLayerController::moveDestination mDest;
		bool mHack;
};

#endif // WBGRAPHICSITEMZLEVELUNDOCOMMAND_H
