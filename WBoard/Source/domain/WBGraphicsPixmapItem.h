#ifndef WBGRAPHICSPIXMAPITEM_H_
#define WBGRAPHICSPIXMAPITEM_H_

#include <QtWidgets>

#include "core/WB.h"

#include "WBItem.h"

class WBGraphicsItemDelegate;

class WBGraphicsPixmapItem : public QObject, public QGraphicsPixmapItem, public WBItem, public WBGraphicsItem
{
    Q_OBJECT

public:
    WBGraphicsPixmapItem(QGraphicsItem* parent = 0);
    virtual ~WBGraphicsPixmapItem();

    enum { Type = WBGraphicsItemType::PixmapItemType };

    virtual int type() const
    {
        return Type;
    }
    virtual WBItem* deepCopy() const;

    virtual void copyItemParameters(WBItem *copy) const;

    virtual WBGraphicsScene* scene();

    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)

    void setOpacity(qreal op);
    qreal opacity() const;

    virtual void clearSource();

    virtual void setUuid(const QUuid &pUuid);

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);
};

#endif /* WBGRAPHICSPIXMAPITEM_H_ */
