#ifndef WBGRAPHICSCURTAINITEM_H_
#define WBGRAPHICSCURTAINITEM_H_

#include <QtWidgets>

#include "core/WB.h"

#include "domain/WBItem.h"

class WBGraphicsItemDelegate;

class WBGraphicsCurtainItem : public QObject, public QGraphicsRectItem, public WBItem, public WBGraphicsItem
{
    Q_OBJECT

public:
    WBGraphicsCurtainItem(QGraphicsItem* parent = 0);
    virtual ~WBGraphicsCurtainItem();

    enum { Type = WBGraphicsItemType::CurtainItemType };

    virtual int type() const
    {
        return Type;
    }

    virtual WBItem* deepCopy() const;
    virtual void copyItemParameters(WBItem *copy) const;

    void triggerRemovedSignal();
    virtual void clearSource(){;}

    virtual void setUuid(const QUuid &pUuid);

    signals:
    void removed();

    protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    QColor  drawColor() const;
    QColor  opaqueControlColor() const;

//        WBGraphicsItemDelegate* mDelegate;

    static const QColor sDrawColor;
    static const QColor sDarkBackgroundDrawColor;
    static const QColor sOpaqueControlColor;
    static const QColor sDarkBackgroundOpaqueControlColor;
};

#endif /* WBGRAPHICSCURTAINITEM_H_ */
