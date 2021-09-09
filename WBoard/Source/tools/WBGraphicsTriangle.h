#ifndef WBGRAPHICSTRIANGLE_H_
#define WBGRAPHICSTRIANGLE_H_

#include <QtWidgets>
#include <QtSvg>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsPolygonItem>

#include "core/WB.h"
#include "domain/WBItem.h"
#include "tools/WBAbstractDrawRuler.h"

class WBGraphicsScene;
class WBItem;

class WBGraphicsTriangle : public WBAbstractDrawRuler, public QGraphicsPolygonItem, public WBItem
{
    Q_OBJECT

public:
    WBGraphicsTriangle();
    virtual ~WBGraphicsTriangle();

    enum { Type = WBGraphicsItemType::TriangleItemType };

    virtual int type() const
    {
        return Type;
    }

    virtual WBItem* deepCopy(void) const;
    virtual void copyItemParameters(WBItem *copy) const;

    virtual void StartLine(const QPointF& scenePos, qreal width);
    virtual void DrawLine(const QPointF& position, qreal width);
    virtual void EndLine();

    enum UBGraphicsTriangleOrientation
    {
        BottomLeft = 0,
        BottomRight,
        TopLeft,
        TopRight
    };

    static UBGraphicsTriangleOrientation orientationFromStr(QStringRef& str)
    {
        if (str == "BottomLeft") return BottomLeft;
        if (str == "BottomRight") return BottomRight;
        if (str == "TopLeft") return TopLeft;
        if (str == "TopRight") return TopRight;
        return sDefaultOrientation;
    }
    static QString orientationToStr(UBGraphicsTriangleOrientation orientation)
    {
        QString result;
        if (orientation == 0) result = "BottomLeft";
        else if (orientation == 1) result = "BottomRight";
        else if (orientation == 2) result = "TopLeft";
        else if (orientation == 3) result = "TopRight";

        return result;
    }

    void setRect(const QRectF &rect, UBGraphicsTriangleOrientation orientation)
    {
        setRect(rect.x(), rect.y(), rect.width(), rect.height(), orientation);
    }
    void setRect(qreal x, qreal y, qreal w, qreal h, UBGraphicsTriangleOrientation orientation);
    void setOrientation(UBGraphicsTriangleOrientation orientation);
    UBGraphicsTriangleOrientation getOrientation() const {return mOrientation;}
    QRectF rect() const {return boundingRect();}

    WBGraphicsScene* scene() const;

protected:
    void updateResizeCursor();

    virtual void paint (QPainter *painter, const QStyleOptionGraphicsItem *styleOption, QWidget *widget);
    virtual QPainterPath shape() const;

    virtual void rotateAroundCenter(qreal angle);

    virtual QPointF rotationCenter() const;

    virtual QRectF closeButtonRect() const;
    QPolygonF resize1Polygon() const;
    QPolygonF resize2Polygon() const;
    QRectF hFlipRect() const;
    QRectF vFlipRect() const;
    QRectF rotateRect() const;

    QRectF bounding_Rect() const;

    QCursor resizeCursor1() const;
    QCursor resizeCursor2() const;

    QCursor flipCursor() const;

    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
    virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *event);
    void paintGraduations(QPainter *painter);

private:
    QCursor mResizeCursor1;
    QCursor mResizeCursor2;

    QTransform calculateRotationTransform();
    qreal angle;
    void rotateAroundCenter(QTransform& transform, QPointF center);

    bool mResizing1;
    bool mResizing2;
    bool mRotating;
    QRect lastRect;

    QPoint lastPos;

    // Save the last bounds rect
    QRectF bounds_rect;

    QGraphicsSvgItem* mHFlipSvgItem;
    QGraphicsSvgItem* mVFlipSvgItem;
    QGraphicsSvgItem* mRotateSvgItem;

    static const QRect sDefaultRect;
    static const UBGraphicsTriangleOrientation sDefaultOrientation;

    UBGraphicsTriangleOrientation mOrientation;

    QPointF A1, B1, C1, A2, B2, C2;
    qreal C;
    qreal W1, H1;
    QPointF CC;
    void calculatePoints(const QRectF& rect);

    bool mShouldPaintInnerTriangle;

    static const int d = 70;
    static const int sArrowLength = 30;
    static const int sMinWidth = 240;
    static const int sMinHeight = 120;
    qreal mStrokeWidth;

    bool contains(const QRectF &rect, bool strict = true);
};

#endif /* WBGRAPHICSTRIANGLE_H_ */
