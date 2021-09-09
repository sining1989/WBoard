#ifndef UBGRAPHICSPROTRACTOR_H_
#define UBGRAPHICSPROTRACTOR_H_

#include <QtWidgets>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsView>
#include <QtSvg>

#include "core/WB.h"
#include "tools/WBAbstractDrawRuler.h"
#include "domain/WBItem.h"

#include "frameworks/WBGeometryUtils.h"


class WBGraphicsScene;

class WBGraphicsProtractor : public WBAbstractDrawRuler, public QGraphicsEllipseItem, public WBItem
{

    Q_OBJECT

public:
    WBGraphicsProtractor ();
    enum Tool {None, Move, Resize, Rotate, Reset, Close, MoveMarker};

    qreal angle () { return mStartAngle; }
    qreal markerAngle () { return mCurrentAngle; }
    void  setAngle (qreal angle) { mStartAngle = angle; setStartAngle(mStartAngle * 16); }
    void  setMarkerAngle (qreal angle) { mCurrentAngle = angle; }

    virtual WBItem* deepCopy() const;
    virtual void copyItemParameters(WBItem *copy) const;

    enum { Type = WBGraphicsItemType::ProtractorItemType };

    virtual int type() const
    {
        return Type;
    }

protected:
    virtual void paint (QPainter *painter, const QStyleOptionGraphicsItem *styleOption, QWidget *widget);

    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    virtual void   mousePressEvent (QGraphicsSceneMouseEvent *event);
    virtual void    mouseMoveEvent (QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent (QGraphicsSceneMouseEvent *event);
    virtual void   hoverEnterEvent (QGraphicsSceneHoverEvent *event);
    virtual void   hoverLeaveEvent (QGraphicsSceneHoverEvent *event);
    virtual void    hoverMoveEvent (QGraphicsSceneHoverEvent *event);
    virtual QPainterPath shape() const;
    QRectF boundingRect() const;
    void paintGraduations(QPainter *painter);        


private:
    void paintButtons (QPainter *painter);
    void paintAngleMarker (QPainter *painter);
    Tool toolFromPos (QPointF pos);
    qreal antiScale () const;
    WBGraphicsScene*            scene() const;
    QBrush                  fillBrush() const;

    QSizeF buttonSizeReference () const{return QSizeF(radius() / 10, mCloseSvgItem->boundingRect().height() * radius()/(10 * mCloseSvgItem->boundingRect().width()));}
    QSizeF markerSizeReference () const{return QSizeF(radius() / 10, mMarkerSvgItem->boundingRect().height() * radius()/(10 * mMarkerSvgItem->boundingRect().width()));}
    QRectF    resetButtonRect () const;

    QRectF    closeButtonRect () const;
    QRectF    resizeButtonRect () const;
    QRectF    rotateButtonRect () const;
    QRectF    markerButtonRect () const;

    inline qreal radius () const{return rect().height() / 2 - 20;}

    QPointF mPreviousMousePos;
    Tool    mCurrentTool;
    bool    mShowButtons;
    qreal   mCurrentAngle;
    qreal   mSpan;
    qreal   mStartAngle;
    qreal   mScaleFactor;

    QGraphicsSvgItem* mResetSvgItem;
    QGraphicsSvgItem* mResizeSvgItem;
    QGraphicsSvgItem* mMarkerSvgItem;
    QGraphicsSvgItem* mRotateSvgItem;

    static const QRectF sDefaultRect;
    static const qreal minRadius;

    virtual void rotateAroundCenter(qreal angle);
    virtual QPointF    rotationCenter() const;

    int    sFillTransparency;
    int    sDrawTransparency;
};

#endif /* UBGRAPHICSPROTRACTOR_H_ */
