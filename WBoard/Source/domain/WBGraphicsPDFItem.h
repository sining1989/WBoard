#ifndef WBGRAPHICSPDFITEM_H_
#define WBGRAPHICSPDFITEM_H_

#include <QtWidgets>

#include "WBItem.h"
#include "core/WB.h"
#include "pdf/GraphicsPDFItem.h"

class WBGraphicsItemDelegate;
class WBGraphicsPixmapItem;

class WBGraphicsPDFItem: public GraphicsPDFItem, public WBItem, public WBGraphicsItem
{
	Q_OBJECT
public:
    WBGraphicsPDFItem(PDFRenderer *renderer, int pageNumber, QGraphicsItem* parent = 0);
    virtual ~WBGraphicsPDFItem();

    enum { Type = WBGraphicsItemType::PDFItemType };

    virtual int type() const
    {
        return Type;
    }

    virtual WBItem* deepCopy() const;

    virtual void copyItemParameters(WBItem *copy) const;

    virtual void setRenderingQuality(RenderingQuality pRenderingQuality);

    virtual WBGraphicsScene* scene();

    virtual WBGraphicsPixmapItem* toPixmapItem() const;

    virtual void clearSource(){;}
    virtual void setUuid(const QUuid &pUuid);

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

};

#endif /* WBGRAPHICSPDFITEM_H_ */
