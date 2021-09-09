#ifndef WBGRAPHICSGROUPCONTAINERITEM_H
#define WBGRAPHICSGROUPCONTAINERITEM_H

#include <QGraphicsItem>

#include "domain/WBItem.h"
#include "frameworks/WBCoreGraphicsScene.h"

class WBGraphicsGroupContainerItem : public QGraphicsItem, public WBItem, public WBGraphicsItem
{
public:
    WBGraphicsGroupContainerItem (QGraphicsItem *parent = 0);
    virtual ~WBGraphicsGroupContainerItem();

    void addToGroup(QGraphicsItem *item);
    void removeFromGroup(QGraphicsItem *item);
    void setCurrentItem(QGraphicsItem *item){mCurrentItem = item;}
    QGraphicsItem *getCurrentItem() const {return mCurrentItem;}
    void deselectCurrentItem();

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    virtual WBCoreGraphicsScene *corescene();
    WBGraphicsGroupContainerItem *deepCopyNoChildDuplication() const;
    virtual WBGraphicsGroupContainerItem *deepCopy() const;
    virtual void copyItemParameters(WBItem *copy) const;

    enum { Type = WBGraphicsItemType::groupContainerType };

    virtual int type() const
    {
        return Type;
    }

    virtual void setUuid(const QUuid &pUuid);
    void destroy(bool canUndo = true);

    virtual void clearSource();

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    void pRemoveFromGroup(QGraphicsItem *item);

private:
    QRectF itemsBoundingRect;
    QGraphicsItem *mCurrentItem;

};

#endif // WBGRAPHICSGROUPCONTAINERITEM_H
