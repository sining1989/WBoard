#ifndef WBGRAPHICSDELEGATEFRAME_H_
#define WBGRAPHICSDELEGATEFRAME_H_

#include <QtWidgets>
#include "core/WB.h"

class QGraphicsSceneMouseEvent;
class WBGraphicsItemDelegate;
class QGraphicsSvgItem;

class WBGraphicsDelegateFrame: public QGraphicsRectItem, public QObject
{
    public:
        WBGraphicsDelegateFrame(WBGraphicsItemDelegate* pDelegate, QRectF pRect, qreal pFrameWidth, bool respectRatio = true, bool hasTitleBar = false);
        virtual ~WBGraphicsDelegateFrame();

        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

        QPainterPath shape() const;

        virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
        QPointF getFixedPointFromPos();
        QSizeF getResizeVector(qreal moveX, qreal moveY);
        QSizeF resizeDelegate(qreal moveX, qreal moveY);
        virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
        virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

        void positionHandles();
        void setVisible(bool visible);

        virtual void setAntiScale(qreal pAntiScale);

        enum OperationMode {Scaling, Resizing, ResizingHorizontally};
        void setOperationMode(OperationMode pMode) {mOperationMode = pMode;}
        bool isResizing(){return mResizing;}
        void moveLinkedItems(QLineF movingVector, bool bLinked = false);
        void prepareFramesToMove(QList<WBGraphicsDelegateFrame *> framesToMove);
        void prepareLinkedFrameToMove();
        QList<WBGraphicsDelegateFrame *> getLinkedFrames();

    private:
        QRectF bottomRightResizeGripRect() const;
        QRectF bottomResizeGripRect() const;
        QRectF leftResizeGripRect() const;
        QRectF rightResizeGripRect() const;
        QRectF topResizeGripRect() const;
        QRectF rotateButtonBounds() const;

        inline bool resizingBottomRight () const { return mCurrentTool == ResizeBottomRight; }
        inline bool resizingBottom ()  const { return mCurrentTool == ResizeBottom; }
        inline bool resizingRight () const { return mCurrentTool == ResizeRight; }
        inline bool resizingLeft () const { return mCurrentTool == ResizeLeft; }
        inline bool resizingTop () const { return mCurrentTool == ResizeTop; }
        inline bool rotating () const { return mCurrentTool == Rotate; }
        inline bool moving () const { return mCurrentTool == Move; }
        void setCursorFromAngle(QString angle);
        bool canResizeBottomRight(qreal width, qreal height, qreal scaleFactor);

        QTransform buildTransform ();
        void  updateResizeCursors ();
        void  initializeTransform ();

        enum FrameTool {None, Move, Rotate, ResizeBottomRight, ResizeTop, ResizeRight, ResizeBottom, ResizeLeft};
        FrameTool toolFromPos (QPointF pos);
        void refreshGeometry();

        FrameTool mCurrentTool;
        WBGraphicsItemDelegate* mDelegate;

        bool mVisible;
        qreal mFrameWidth;
        qreal mNominalFrameWidth;
        bool mRespectRatio;

        qreal mAngle;
        qreal mAngleOffset;
        qreal mTotalScaleX;
        qreal mTotalScaleY;
        qreal mScaleX;
        qreal mScaleY;
        qreal mTranslateX;
        qreal mTranslateY;
        qreal mTotalTranslateX;
        qreal mTotalTranslateY;
        qreal mAngleTolerance;
        QRect mAngleRect;

        QPointF mStartingPoint;
        QTransform mInitialTransform;
        QSizeF mOriginalSize;
        QPointF mFixedPoint;

        QGraphicsSvgItem* mBottomRightResizeGripSvgItem;
        QGraphicsSvgItem* mBottomResizeGripSvgItem;
        QGraphicsSvgItem* mLeftResizeGripSvgItem;
        QGraphicsSvgItem* mRightResizeGripSvgItem;
        QGraphicsSvgItem* mTopResizeGripSvgItem;
        QGraphicsSvgItem* mRotateButton;

        QGraphicsRectItem* mBottomRightResizeGrip;
        QGraphicsRectItem* mBottomResizeGrip;
        QGraphicsRectItem* mLeftResizeGrip;
        QGraphicsRectItem* mRightResizeGrip;
        QGraphicsRectItem* mTopResizeGrip;

        OperationMode mOperationMode;

        QGraphicsItem* delegated();
        bool mFlippedX;
        bool mFlippedY;
        bool mMirrorX;
        bool mMirrorY;
        bool mResizing;
        bool mMirroredXAtStart;
        bool mMirroredYAtStart;
        qreal mTitleBarHeight;
        qreal mNominalTitleBarHeight;

        QList<WBGraphicsDelegateFrame *> mLinkedFrames;
};
#endif /* WBGRAPHICSDELEGATEFRAME_H_ */
