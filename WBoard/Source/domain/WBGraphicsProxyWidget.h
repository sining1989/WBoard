#ifndef WBGRAPHICSPROXYWIDGET_H_
#define WBGRAPHICSPROXYWIDGET_H_

#include <QtWidgets>

#include "WBItem.h"
#include "WBResizableGraphicsItem.h"

class WBGraphicsItemDelegate;

class WBGraphicsProxyWidget: public QGraphicsProxyWidget, public WBItem, public WBResizableGraphicsItem, public WBGraphicsItem
{
    public:
        virtual ~WBGraphicsProxyWidget();

        virtual void resize(qreal w, qreal h);
        virtual void resize(const QSizeF & size);

        virtual QSizeF size() const;

        virtual WBGraphicsScene* scene();

        virtual void setUuid(const QUuid &pUuid);

    protected:
        WBGraphicsProxyWidget(QGraphicsItem* parent = 0);

        virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
        virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
        virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
        virtual void wheelEvent(QGraphicsSceneWheelEvent *event);
        virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
        virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

        virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
        virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

};

#endif /* WBGRAPHICSPROXYWIDGET_H_ */
