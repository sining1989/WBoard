#ifndef WBGRAPHICSCURTAINITEMDELEGATE_H_
#define WBGRAPHICSCURTAINITEMDELEGATE_H_

#include <QtWidgets>

#include <QtSvg>

#include "core/WB.h"
#include "domain/WBGraphicsItemDelegate.h"

class QGraphicsSceneMouseEvent;
class QGraphicsItem;
class WBGraphicsCurtainItem;

class WBGraphicsCurtainItemDelegate : public WBGraphicsItemDelegate
{
    Q_OBJECT

public:
    WBGraphicsCurtainItemDelegate(WBGraphicsCurtainItem* pDelegated, QObject * parent = 0);
    virtual ~WBGraphicsCurtainItemDelegate();

    virtual bool mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value);
    virtual void positionHandles();

    virtual void init();

public slots:
    virtual void remove(bool checked, bool canUndo = true);

};

#endif /* WBGRAPHICSCURTAINITEMDELEGATE_H_ */
