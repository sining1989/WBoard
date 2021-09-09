#ifndef WBGRAPHICSTEXTITEM_H_
#define WBGRAPHICSTEXTITEM_H_

#include <QtWidgets>
#include "WBItem.h"
#include "core/WB.h"
#include "WBResizableGraphicsItem.h"

class WBGraphicsItemDelegate;
class WBGraphicsScene;

class WBGraphicsTextItem : public QGraphicsTextItem, public WBItem, public WBResizableGraphicsItem, public WBGraphicsItem
{
    Q_OBJECT

    public:
        WBGraphicsTextItem(QGraphicsItem * parent = 0);
        virtual ~WBGraphicsTextItem();

        enum { Type = WBGraphicsItemType::TextItemType };

        virtual int type() const
        {
            return Type;
        }

        virtual WBItem* deepCopy() const;

        virtual void copyItemParameters(WBItem *copy) const;

        virtual WBGraphicsScene* scene();

        virtual QRectF boundingRect() const;
        virtual QPainterPath shape() const;

        void setTextWidth(qreal width);
        void setTextHeight(qreal height);
        qreal textHeight() const;
        qreal pixelsPerPoint() const;

        void contentsChanged();

        virtual void resize(qreal w, qreal h);

        virtual QSizeF size() const;

        static QColor lastUsedTextColor;

        QColor colorOnDarkBackground() const
        {
            return mColorOnDarkBackground;
        }

        void setColorOnDarkBackground(QColor pColorOnDarkBackground)
        {
            mColorOnDarkBackground = pColorOnDarkBackground;
        }

        QColor colorOnLightBackground() const
        {
            return mColorOnLightBackground;
        }

        void setColorOnLightBackground(QColor pColorOnLightBackground)
        {
            mColorOnLightBackground = pColorOnLightBackground;
        }

        virtual void clearSource(){;}
        virtual void setUuid(const QUuid &pUuid);
        void activateTextEditor(bool activate);
        void setSelected(bool selected);
        void recolor();

        QString mTypeTextHereLabel;

    signals:
        void textUndoCommandAdded(WBGraphicsTextItem *textItem);

    private slots:
        void undoCommandAdded();
        void documentSizeChanged(const QSizeF & newSize);

    private:
        virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
        virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
        virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

        virtual void keyPressEvent(QKeyEvent *event);
        virtual void keyReleaseEvent(QKeyEvent *event);

        virtual void dragMoveEvent(QGraphicsSceneDragDropEvent *event);

        virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

        virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

        qreal mTextHeight;

        int mMultiClickState;
        QTime mLastMousePressTime;

        QColor mColorOnDarkBackground;
        QColor mColorOnLightBackground;
        bool isActivatedTextEditor;
};

#endif /* WBGRAPHICSTEXTITEM_H_ */
