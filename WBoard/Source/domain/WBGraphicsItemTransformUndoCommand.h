#ifndef WBGRAPHICSITEMTRANSFORMUNDOCOMMAND_H_
#define WBGRAPHICSITEMTRANSFORMUNDOCOMMAND_H_

#include <QtWidgets>

#include "WBUndoCommand.h"

class WBGraphicsItemTransformUndoCommand : public WBUndoCommand
{
    public:
        WBGraphicsItemTransformUndoCommand(QGraphicsItem* pItem,
                                                const QPointF& prevPos,
                                                const QTransform& prevTransform,
                                                const qreal& prevZValue,
                                                const QSizeF& prevSize = QSizeF(),
                                                bool setToBackground = false);
        virtual ~WBGraphicsItemTransformUndoCommand();

        virtual int getType() const { return WBUndoType::undotype_GRAPHICITEMTRANSFORM; }

    protected:
        virtual void undo();
        virtual void redo();

    private:
        QGraphicsItem* mItem;
        QTransform mPreviousTransform;
        QTransform mCurrentTransform;
        QPointF mPreviousPosition;
        QPointF mCurrentPosition;
        QSizeF mPreviousSize;
        QSizeF mCurrentSize;

        qreal mPreviousZValue;
        qreal mCurrentZValue;

        bool mSetToBackground;

};

#endif /* WBGRAPHICSITEMTRANSFORMUNDOCOMMAND_H_ */
