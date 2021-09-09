#ifndef WBGRAPHICSSTROKESGROUP_H
#define WBGRAPHICSSTROKESGROUP_H

#include <QGraphicsItemGroup>
#include <QGraphicsSceneMouseEvent>

#include "core/WB.h"
#include "WBItem.h"

class WBGraphicsStrokesGroup : public QObject, public QGraphicsItemGroup, public WBItem, public WBGraphicsItem
{
    Q_OBJECT
public:
    enum colorType {
        currentColor = 0,
        colorOnLightBackground,
        colorOnDarkBackground
    };

    WBGraphicsStrokesGroup(QGraphicsItem* parent = 0);
    ~WBGraphicsStrokesGroup();
    virtual WBItem* deepCopy() const;
    virtual void copyItemParameters(WBItem *copy) const;
    enum { Type = WBGraphicsItemType::StrokeItemType };
    virtual int type() const
    {
        return Type;
    }
    virtual void setUuid(const QUuid &pUuid);
    void setColor(const QColor &color, colorType pColorType = currentColor);
    QColor color(colorType pColorType = currentColor) const;

protected:
    virtual QPainterPath shape () const;

    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    bool debugTextEnabled;
    QGraphicsSimpleTextItem * mDebugText;
};

#endif // WBGRAPHICSSTROKESGROUP_H
