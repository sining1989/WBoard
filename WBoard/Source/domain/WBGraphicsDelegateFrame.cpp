#include "WBGraphicsDelegateFrame.h"

#include <QtWidgets>
#include <QtSvg>

#include "core/WBApplication.h"
#include "core/WBSettings.h"

#include "board/WBBoardController.h"
#include "board/WBBoardView.h"

#include "domain/WBGraphicsItemDelegate.h"
#include "domain/WBGraphicsScene.h"
#include "domain/WBGraphicsProxyWidget.h"

#include "gui/WBResources.h"

#include "core/memcheck.h"

WBGraphicsDelegateFrame::WBGraphicsDelegateFrame(WBGraphicsItemDelegate* pDelegate, QRectF pRect, qreal pFrameWidth, bool respectRatio, bool hasTitleBar)
    : QGraphicsRectItem(), QObject(pDelegate)
    , mCurrentTool(None)
    , mDelegate(pDelegate)
    , mVisible(true)
    , mFrameWidth(pFrameWidth)
    , mNominalFrameWidth(pFrameWidth)
    , mRespectRatio(respectRatio)
    , mAngle(0)
    , mAngleOffset(0)
    , mTotalScaleX(-1)
    , mTotalScaleY(-1)
    , mTranslateX(0)
    , mTranslateY(0)
    , mTotalTranslateX(0)
    , mTotalTranslateY(0)
    , mOperationMode(Scaling)
    , mFlippedX(false)
    , mFlippedY(false)
    , mMirrorX(false)
    , mMirrorY(false)
    , mTitleBarHeight(hasTitleBar ? 20 :0)
    , mNominalTitleBarHeight(hasTitleBar ? 20:0)
{
    mAngleTolerance = WBSettings::settings()->angleTolerance->get().toReal();

    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    setAcceptedMouseButtons(Qt::LeftButton);
    setRect(pRect.adjusted(mFrameWidth, mFrameWidth + mTitleBarHeight, mFrameWidth * -1, mFrameWidth * -1));

    setBrush(QBrush(WBSettings::paletteColor));
    setPen(Qt::NoPen);
    setData(WBGraphicsItemData::ItemLayerType, QVariant(WBItemLayerType::Control));

    mBottomRightResizeGripSvgItem = new QGraphicsSvgItem(":/images/resize.svg", this);
    mBottomResizeGripSvgItem = new QGraphicsSvgItem(":/images/resizeBottom.svg", this);
    mLeftResizeGripSvgItem = new QGraphicsSvgItem(":/images/resizeLeft.svg", this);
    mRightResizeGripSvgItem = new QGraphicsSvgItem(":/images/resizeRight.svg", this);
    mTopResizeGripSvgItem = new QGraphicsSvgItem(":/images/resizeTop.svg", this);

    mBottomRightResizeGrip = new QGraphicsRectItem(this);
    mBottomRightResizeGrip->setPen(Qt::NoPen);
    mBottomResizeGrip = new QGraphicsRectItem(this);
    mBottomResizeGrip->setPen(Qt::NoPen);
    mLeftResizeGrip = new QGraphicsRectItem(this);
    mLeftResizeGrip->setPen(Qt::NoPen);
    mRightResizeGrip = new QGraphicsRectItem(this);
    mRightResizeGrip->setPen(Qt::NoPen);
    mTopResizeGrip = new QGraphicsRectItem(this);
    mTopResizeGrip->setPen(Qt::NoPen);

    mRotateButton = new QGraphicsSvgItem(":/images/rotate.svg", this);
    mRotateButton->setCursor(WBResources::resources()->rotateCursor);
    mRotateButton->setVisible(mDelegate->testUBFlags(GF_REVOLVABLE));

    updateResizeCursors();

    setAntiScale(1.0);

    positionHandles();

    this->setAcceptHoverEvents(true);
}


WBGraphicsDelegateFrame::~WBGraphicsDelegateFrame()
{
    // NOOP
}

void WBGraphicsDelegateFrame::setAntiScale(qreal pAntiScale)
{
    mFrameWidth = mNominalFrameWidth * pAntiScale;
    mTitleBarHeight = mNominalTitleBarHeight * pAntiScale;

    QTransform tr;
    tr.scale(pAntiScale, pAntiScale);

    mBottomRightResizeGripSvgItem->setTransform(tr);
    mBottomResizeGripSvgItem->setTransform(tr);
    mLeftResizeGripSvgItem->setTransform(tr);
    mRightResizeGripSvgItem->setTransform(tr);
    mTopResizeGripSvgItem->setTransform(tr);
    mRotateButton->setTransform(tr);
}


void WBGraphicsDelegateFrame::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);


    QPainterPath path;
    path.addRoundedRect(rect(), mFrameWidth / 2, mFrameWidth / 2);

    if (rect().width() > 1 && rect().height() > 1)
    {
        QPainterPath extruded;
        extruded.addRect(rect().adjusted(mFrameWidth, mFrameWidth + mTitleBarHeight, (mFrameWidth * -1), (mFrameWidth * -1)));
        path = path.subtracted(extruded);
    }

    painter->fillPath(path, brush());
}

QPainterPath WBGraphicsDelegateFrame::shape() const
{
    QPainterPath path;

    //We do not use the rounded rect here because we want the bottom right corner
    //to be included in the frame (for resize grip handling : #702)
    path.addRect(rect());

    if (rect().width() > 0 && rect().height() > 0)
    {
        QPainterPath extruded;
        extruded.addRect(rect().adjusted(mFrameWidth, mFrameWidth + mTitleBarHeight, mFrameWidth * -1, mFrameWidth * -1));
        path = path.subtracted(extruded);
    }

    return path;
}


void WBGraphicsDelegateFrame::initializeTransform()
{
    QTransform itemTransform = delegated()->sceneTransform();
    QRectF itemRect = delegated()->boundingRect();
    QPointF topLeft = itemTransform.map(itemRect.topLeft());
    QPointF topRight = itemTransform.map(itemRect.topRight());
    QPointF  bottomLeft = itemTransform.map(itemRect.bottomLeft());

    qreal horizontalFlip = (topLeft.x() > topRight.x()) ? -1 : 1;
    mMirrorX = horizontalFlip < 0 ;
    if(horizontalFlip < 0){
        // why this is because of the way of calculating the translations that checks which side is the most is the
        // nearest instead of checking which one is the left side.
        QPointF tmp = topLeft;
        topLeft = topRight;
        topRight = tmp;

        // because of the calculation of the height is done by lenght and not deltaY
        bottomLeft = itemTransform.map(itemRect.bottomRight());
    }

    qreal verticalFlip = (bottomLeft.y() < topLeft.y()) ? -1 : 1;
    // not sure that is usefull
    mMirrorY = verticalFlip < 0;
    if(verticalFlip < 0 && !mMirrorX){
        topLeft = itemTransform.map(itemRect.bottomLeft());
        topRight = itemTransform.map(itemRect.bottomRight());
        bottomLeft = itemTransform.map(itemRect.topLeft());
    }

    QLineF topLine(topLeft, topRight);
    QLineF leftLine(topLeft, bottomLeft);
    qreal width = topLine.length();
    qreal height = leftLine.length();

    mAngle = topLine.angle();

    // the fact the the length is used we loose the horizontalFlip information
    // a better way to do this is using DeltaX that preserve the direction information.
    mTotalScaleX = (width / itemRect.width()) * horizontalFlip;
    mTotalScaleY = height / itemRect.height() * verticalFlip;

    QTransform tr;
    QPointF center = delegated()->boundingRect().center();
    tr.translate(center.x() * mTotalScaleX, center.y() * mTotalScaleY);
    tr.rotate(-mAngle);
    tr.translate(-center.x() * mTotalScaleX, -center.y() * mTotalScaleY);
    tr.scale(mTotalScaleX, mTotalScaleY);

    mTotalTranslateX = delegated()->transform().dx() - tr.dx();
    mTotalTranslateY = delegated()->transform().dy() - tr.dy();
}


void WBGraphicsDelegateFrame::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    mDelegate->startUndoStep();

    mStartingPoint = event->scenePos();

    initializeTransform();

    mScaleX = 1;
    mScaleY = 1;
    mTranslateX = 0;
    mTranslateY = 0;
    mAngleOffset = 0;

    mInitialTransform = buildTransform();
    mOriginalSize = delegated()->boundingRect().size();

    mCurrentTool = toolFromPos(event->pos());
    setCursorFromAngle(QString::number((int)mAngle % 360));
    event->accept();
}

void WBGraphicsDelegateFrame::setCursorFromAngle(QString angle)
{
    if (mCurrentTool == Rotate)
    {
        QWidget *controlViewport = WBApplication::boardController->controlView()->viewport();

        QSize cursorSize(45,30);


        QImage mask_img(cursorSize, QImage::Format_Mono);
        mask_img.fill(0xff);
        QPainter mask_ptr(&mask_img);
        mask_ptr.setBrush( QBrush( QColor(0, 0, 0) ) );
        mask_ptr.drawRoundedRect(0,0, cursorSize.width()-1, cursorSize.height()-1, 6, 6);
        QBitmap bmpMask = QBitmap::fromImage(mask_img);


        QPixmap pixCursor(cursorSize);
        pixCursor.fill(QColor(Qt::white));

        QPainter painter(&pixCursor);

        painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        painter.setBrush(QBrush(Qt::white));
        painter.setPen(QPen(QColor(Qt::black)));
        painter.drawRoundedRect(1,1,cursorSize.width()-2,cursorSize.height()-2,6,6);
        painter.setFont(QFont("Arial", 10));
        painter.drawText(1,1,cursorSize.width(),cursorSize.height(), Qt::AlignCenter, angle.append(QChar(176)));
        painter.end();

        pixCursor.setMask(bmpMask);
        controlViewport->setCursor(pixCursor);
    }
}


bool WBGraphicsDelegateFrame::canResizeBottomRight(qreal width, qreal height, qreal scaleFactor)
{
    bool res = false;

    if(!mMirrorX && !mMirrorX && ((width * scaleFactor) > 2*mFrameWidth && (height * scaleFactor) > 2*mFrameWidth)){
        res = true;
    }else if(mMirrorX && !mMirrorY && (-width * scaleFactor) > 2*mFrameWidth && (height*scaleFactor) > 2*mFrameWidth){
        res = true;
    }else if(!mMirrorX && mMirrorY && (width * scaleFactor) > 2*mFrameWidth && (-height*scaleFactor) > 2*mFrameWidth){
        res = true;
    }else if(mMirrorX && mMirrorY && (-width * scaleFactor) > 2*mFrameWidth && (-height*scaleFactor) > 2*mFrameWidth){
        res = true;
    }

    return res;
}

QPointF WBGraphicsDelegateFrame::getFixedPointFromPos()
{
    QPointF fixedPoint;
    if (!moving() && !rotating())
    {
        if (resizingTop())
        {
            if (mMirrorX && mMirrorY)
            {
                if ((0 < mAngle) && (mAngle < 90))
                    fixedPoint = delegated()->sceneBoundingRect().topLeft();
                else
                    fixedPoint = delegated()->sceneBoundingRect().topRight();
            }
            else
            {
                if ((0 < mAngle) && (mAngle <= 90))
                    fixedPoint = delegated()->sceneBoundingRect().bottomRight();
                else
                    fixedPoint = delegated()->sceneBoundingRect().bottomLeft();
            }
        }
        else if (resizingLeft())
        {
            if (mMirrorX && mMirrorY)
            {
                if ((0 < mAngle) && (mAngle < 90))
                    fixedPoint = delegated()->sceneBoundingRect().bottomLeft();
                else
                    fixedPoint = delegated()->sceneBoundingRect().topLeft();
            }
            else
            {
                if ((0 < mAngle) && (mAngle <= 90))
                    fixedPoint = delegated()->sceneBoundingRect().topRight();
                else
                    fixedPoint = delegated()->sceneBoundingRect().bottomRight();
            }
        }
    }
    return fixedPoint;
}


QSizeF WBGraphicsDelegateFrame::getResizeVector(qreal moveX, qreal moveY)
{
    qreal dPosX = 0;
    qreal dPosY = 0;

    if (resizingTop())
    {
        if (mMirrorX && mMirrorY)
            dPosY = moveY;
        else
            dPosY = -moveY;
    }
    else if (resizingLeft())
    {
        if (mMirrorX && mMirrorY)
            dPosX = moveX;
        else
            dPosX = -moveX;
    }

    else if (resizingRight())
        dPosX = (mMirrorX) ?  -moveX : moveX;
    else if (resizingBottom())
        dPosY = mMirrorY ? -moveY : moveY;

    return QSizeF(dPosX, dPosY);
}

QSizeF WBGraphicsDelegateFrame::resizeDelegate(qreal moveX, qreal moveY)
{
    QSizeF incVector;
    mFixedPoint = getFixedPointFromPos();

    WBResizableGraphicsItem* resizableItem = dynamic_cast<WBResizableGraphicsItem*>(delegated());
    if (resizableItem)
    {
        incVector = getResizeVector(moveX, moveY);
        resizableItem->resize(mOriginalSize + incVector);

        if (resizingTop() || resizingLeft() || ((mMirrorX || mMirrorY) && resizingBottomRight()))
        {
            QPointF pos1 = getFixedPointFromPos();
            delegated()->setPos(delegated()->pos()-pos1+mFixedPoint);
        }
    }

    return incVector;
}

void WBGraphicsDelegateFrame::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (None == mCurrentTool)
        return;

    QLineF move = QLineF(mStartingPoint, event->scenePos());
    qreal moveX = (event->pos() - mStartingPoint).x();
    qreal moveY = (event->pos() - mStartingPoint).y();
    qreal width = delegated()->boundingRect().width() * mTotalScaleX;
    qreal height = delegated()->boundingRect().height() * mTotalScaleY;

    if (mOperationMode ==  Scaling)
    {
        if(!rotating())
        {
            mTranslateX = moveX;
            // Perform the resize
            if (resizingBottomRight())
            {
                // -----------------------------------------------------
                // ! We want to keep the aspect ratio with this resize !
                // -----------------------------------------------------
                qreal scaleX;
                qreal scaleY;

                if(!mMirrorX){
                    scaleX = (width + moveX) / width;
                }else{
                    scaleX = (width - moveX) / width;
                }

                if(!mMirrorY){
                    scaleY = (height + moveY) / height;
                }else{
                    scaleY = (height - moveY) / height;
                }

                qreal scaleFactor = (scaleX + scaleY) / 2;

                // Do not allow resizing of image size under frame size
                if (canResizeBottomRight(width, height, scaleFactor))
                {
                    if (mRespectRatio)
                    {
                        mScaleX = scaleFactor;
                        mScaleY = scaleFactor;
                    }
                    else
                    {
                        mScaleX = scaleX;
                        mScaleY = scaleY;
                    }
                }
            }
            else if (resizingLeft() || resizingRight()){
                if(width != 0){
                    qreal scaleX = 0.0;
                    if(resizingLeft()){
                        scaleX = (width - moveX) / width;
                    }else if(resizingRight()){
                        scaleX = (width + moveX) / width;
                    }
                    if(mDelegate->testUBFlags(GF_FLIPPABLE_ALL_AXIS) && qAbs(scaleX) != 0){
                        if((qAbs(width * scaleX)) < 2*mFrameWidth){
                            bool negative = (scaleX < 0)?true:false;
                            if(negative){
                                if(mMirrorX)
                                    scaleX = 2*mFrameWidth/width;
                                else
                                    scaleX = -2*mFrameWidth/width;
                            }else{
                                scaleX = -1;
                                mFlippedX = !mFlippedX;
                            }
                        }
                        mScaleX = scaleX;
                    }else if (scaleX > 1 || (width * scaleX) > 2 * mFrameWidth){
                        mScaleX = scaleX;
                        if(resizingLeft()){
                            mTranslateX = moveX;
                        }
                    }
                }
            }else if(resizingTop() || resizingBottom()){
                if(height != 0){
                    qreal scaleY = 0.0;
                    if(resizingTop()){
                        scaleY = (height - moveY) / height;
                    }else if(resizingBottom()){
                        scaleY = (height + moveY) / height;
                    }

                    if(mDelegate->testUBFlags(GF_FLIPPABLE_ALL_AXIS) && qAbs(scaleY) != 0){
                        if((qAbs(height * scaleY)) < 2*mFrameWidth){
                            bool negative = (scaleY < 0)?true:false;
                            if(negative){
                                if(mMirrorY)
                                    scaleY = 2*mFrameWidth/width;
                                else
                                    scaleY = -2*mFrameWidth/width;
                            }else{
                                scaleY = -1;
                                mFlippedY = !mFlippedY;
                            }
                        }
                        mScaleY = scaleY;
                    }else if (scaleY > 1 || (height * scaleY) > 2 * mFrameWidth)
                    {
                        mScaleY = scaleY;
                        if(resizingTop()){
                            mTranslateY = moveY;
                        }
                    }
                }
            }
        }
    }

    if (rotating())
    {
        mTranslateX = 0;
        mTranslateY = 0;

        QLineF startLine(sceneBoundingRect().center(), event->lastScenePos());
        QLineF currentLine(sceneBoundingRect().center(), event->scenePos());
        mAngle += startLine.angleTo(currentLine);

        if ((int)mAngle % 45 >= 45 - mAngleTolerance || (int)mAngle % 45 <= mAngleTolerance)
        {
            mAngle = qRound(mAngle / 45) * 45;
            mAngleOffset += startLine.angleTo(currentLine);
            if ((int)mAngleOffset % 360 > mAngleTolerance && (int)mAngleOffset % 360 < 360 - mAngleTolerance)
            {
                mAngle += mAngleOffset;
                mAngleOffset = 0;
            }
        }
        else if ((int)mAngle % 30 >= 30 - mAngleTolerance || (int)mAngle % 30 <= mAngleTolerance)
        {
            mAngle = qRound(mAngle / 30) * 30;
            mAngleOffset += startLine.angleTo(currentLine);
            if ((int)mAngleOffset % 360 > mAngleTolerance && (int)mAngleOffset % 360 < 360 - mAngleTolerance)
            {
                mAngle += mAngleOffset;
                mAngleOffset = 0;
            }
        }

        setCursorFromAngle(QString::number((int)mAngle % 360));
    }
    else if (moving())
    {
        mTranslateX = move.dx();
        mTranslateY = move.dy();
        moveLinkedItems(move);
    }

    if (mOperationMode == Scaling || moving() || rotating())
    {
        QTransform tr = buildTransform();

        if (resizingRight() || resizingBottom() || resizingBottomRight())
        {
            // we just detects coordinates of corner before and after scaling and then moves object at diff between them.
            if (resizingBottomRight() && (mMirrorX || mMirrorY))
            {
                if (mFlippedX && !mMirrorX && mFlippedY)// && !mMirrorY)
                {
                    mTranslateX += mInitialTransform.map(delegated()->boundingRect().bottomLeft()).x() - tr.map(delegated()->boundingRect().bottomLeft()).x();
                    mTranslateY += mInitialTransform.map(delegated()->boundingRect().bottomLeft()).y() - tr.map(delegated()->boundingRect().bottomLeft()).y();
                }
                else if ((mFlippedX || mMirrorX) && (mFlippedY || mMirrorY))
                {
                    mTranslateX += mInitialTransform.map(delegated()->boundingRect().bottomRight()).x() - tr.map(delegated()->boundingRect().bottomRight()).x();
                    mTranslateY += mInitialTransform.map(delegated()->boundingRect().bottomRight()).y() - tr.map(delegated()->boundingRect().bottomRight()).y();
                }
                else if (mFlippedX || mMirrorX)
                {
                    mTranslateX += mInitialTransform.map(delegated()->boundingRect().topRight()).x() - tr.map(delegated()->boundingRect().topRight()).x();
                    mTranslateY += mInitialTransform.map(delegated()->boundingRect().topRight()).y() - tr.map(delegated()->boundingRect().topRight()).y();
                }
                else if (mFlippedY || mMirrorY)
                {
                    mTranslateX += mInitialTransform.map(delegated()->boundingRect().bottomLeft()).x() - tr.map(delegated()->boundingRect().bottomLeft()).x();
                    mTranslateY += mInitialTransform.map(delegated()->boundingRect().bottomLeft()).y() - tr.map(delegated()->boundingRect().bottomLeft()).y();
                }
                else
                {
                    mTranslateX += mInitialTransform.map(delegated()->boundingRect().bottomRight()).x() - tr.map(delegated()->boundingRect().bottomRight()).x();
                    mTranslateY += mInitialTransform.map(delegated()->boundingRect().bottomRight()).y() - tr.map(delegated()->boundingRect().bottomRight()).y();
                }
            }
            else
            {
                    mTranslateX += mInitialTransform.map(delegated()->boundingRect().topLeft()).x() - tr.map(delegated()->boundingRect().topLeft()).x();
                    mTranslateY += mInitialTransform.map(delegated()->boundingRect().topLeft()).y() - tr.map(delegated()->boundingRect().topLeft()).y();
            }
        }
        else if (resizingTop() || resizingLeft())
        {
            QPointF bottomRight = tr.map(delegated()->boundingRect().bottomRight());
            QPointF fixedPoint = mInitialTransform.map(delegated()->boundingRect().bottomRight());
            mTranslateX += fixedPoint.x() - bottomRight.x();
            mTranslateY += fixedPoint.y() - bottomRight.y();
        }
        delegated()->setTransform(buildTransform());
    }
    else // resizing/resizing horizontally
    {

        if (resizingBottomRight())
        {
            static QSizeF incV = QSizeF();
            static QSizeF incH = QSizeF();

            if (mMirrorX && mMirrorY)
                mCurrentTool = ResizeTop;
            else
                mCurrentTool = ResizeBottom;

            incV = resizeDelegate(moveX, moveY);
            mOriginalSize += incV;

            if (mMirrorX && mMirrorY)
                mCurrentTool = ResizeLeft;
            else
                mCurrentTool = ResizeRight;

            move = QLineF(event->lastScenePos(), event->scenePos());
            moveX = move.length() * cos((move.angle() - mAngle) * PI / 180);
            moveY = -move.length() * sin((move.angle() - mAngle) * PI / 180);

            mFixedPoint = getFixedPointFromPos();

            incH = resizeDelegate(moveX, moveY);

            mOriginalSize -= incV;
            mOriginalSize += incH;

            mCurrentTool = ResizeBottomRight;
        }
        else
            resizeDelegate(moveX, moveY);
    }
    event->accept();
}

QList<WBGraphicsDelegateFrame *> WBGraphicsDelegateFrame::getLinkedFrames()
{
    QList<WBGraphicsDelegateFrame*> linkedFrames;
    QList<QGraphicsItem*> sItems = mDelegate->delegated()->scene()->selectedItems();
    if (sItems.count())
    {
        sItems.removeAll(delegated());

        foreach(QGraphicsItem *item, sItems)
        {
            WBGraphicsItem *gitem = dynamic_cast<WBGraphicsItem*>(item);
            if (gitem)
                linkedFrames << gitem->Delegate()->frame();
        }
    }
    return linkedFrames;
}

void WBGraphicsDelegateFrame::prepareFramesToMove(QList<WBGraphicsDelegateFrame *> framesToMove)
{
    mLinkedFrames = framesToMove;
    foreach (WBGraphicsDelegateFrame *frame, mLinkedFrames)
    {
        frame->prepareLinkedFrameToMove();
    }
}

void WBGraphicsDelegateFrame::prepareLinkedFrameToMove()
{
    mDelegate->startUndoStep();

    mStartingPoint = QPointF(0,0);

    initializeTransform();

    mScaleX = 1;
    mScaleY = 1;
    mTranslateX = 0;
    mTranslateY = 0;
    mAngleOffset = 0;

    mInitialTransform = buildTransform();

    mCurrentTool = Move;
}

void WBGraphicsDelegateFrame::moveLinkedItems(QLineF movingVector, bool bLinked)
{
    if (bLinked)
    {
        mCurrentTool = Move;

        mTranslateX = movingVector.dx();
        mTranslateY = movingVector.dy();

        delegated()->setTransform(buildTransform(), false);
    }
    else
    {
        foreach(WBGraphicsDelegateFrame* frame, mLinkedFrames)
        {
           frame->moveLinkedItems(movingVector, true);
        }
    }
}

QTransform WBGraphicsDelegateFrame::buildTransform()
{
    QTransform tr;
    QPointF center = delegated()->boundingRect().center();

    // Translate
    tr.translate(mTotalTranslateX + mTranslateX, mTotalTranslateY + mTranslateY);

    // Set angle
    tr.translate(center.x() * mTotalScaleX * mScaleX, center.y() * mTotalScaleY * mScaleY);
    tr.rotate(-mAngle);
    tr.translate(-center.x() * mTotalScaleX * mScaleX, -center.y() * mTotalScaleY * mScaleY);

    // Scale
    tr.scale(mTotalScaleX * mScaleX, mTotalScaleY * mScaleY);
    return tr;
}

void WBGraphicsDelegateFrame::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    updateResizeCursors();

    mDelegate->commitUndoStep();
    mTotalScaleX *= mScaleX;
    mTotalScaleY *= mScaleY;
    mTotalTranslateX += mTranslateX;
    mTotalTranslateY += mTranslateY;
    event->accept();

    mCurrentTool = None;

    QGraphicsRectItem::mouseReleaseEvent(event);

    // Show the buttons
    if(isResizing()){
        mResizing = false;
    }
}

void WBGraphicsDelegateFrame::updateResizeCursors()
{
    QPixmap pix(":/images/cursors/resize.png");
    QTransform tr;

    tr.rotate(-mAngle);
    QCursor resizeCursor  = QCursor(pix.transformed(tr, Qt::SmoothTransformation), pix.width() / 2,  pix.height() / 2);
    mLeftResizeGrip->setCursor(resizeCursor);
    mRightResizeGrip->setCursor(resizeCursor);

    tr.rotate(-90);
    resizeCursor  = QCursor(pix.transformed(tr, Qt::SmoothTransformation), pix.width() / 2,  pix.height() / 2);
    mBottomResizeGrip->setCursor(resizeCursor);
    mTopResizeGrip->setCursor(resizeCursor);

    tr.rotate(-45);
    resizeCursor  = QCursor(pix.transformed(tr, Qt::SmoothTransformation), pix.width() / 2,  pix.height() / 2);
    mBottomRightResizeGrip->setCursor(resizeCursor);
}

void WBGraphicsDelegateFrame::setVisible(bool visible)
{
    mVisible = visible;
    if (mVisible)
       setBrush(QBrush(WBSettings::paletteColor));
    else
       setBrush(Qt::NoBrush);
}

void WBGraphicsDelegateFrame::positionHandles()
{
    QRectF itemRect = delegated()->boundingRect();

    if (mDelegate->getToolBarItem() && mDelegate->getToolBarItem()->isVisibleOnBoard()
        && mDelegate->getToolBarItem()->isShifting())
    {
        QPointF graphicsItemPosition = itemRect.topLeft();
        itemRect.setTopLeft(graphicsItemPosition-QPointF(0,mDelegate->getToolBarItem()->boundingRect().height()* mDelegate->antiScaleRatio()));
    }

    QTransform itemTransform = delegated()->sceneTransform();
    QPointF topLeft = itemTransform.map(itemRect.topLeft());
    QPointF topRight = itemTransform.map(itemRect.topRight());
    QPointF bottomLeft = itemTransform.map(itemRect.bottomLeft());
    QPointF bottomRight = itemTransform.map(itemRect.bottomRight());
    QPointF center = itemTransform.map(itemRect.center());
    int rotateHeight = QLineF(topLeft, bottomLeft).length();

    // Handle the mirroring
    if(topLeft.x() > topRight.x()){
        QPointF topTmp = topRight;
        QPointF bottomTmp = bottomRight;
        topRight = topLeft;
        topLeft = topTmp;
        bottomRight = bottomLeft;
        bottomLeft = bottomTmp;
    }

    if(bottomLeft.y() > topLeft.y()){
        QPointF leftTmp = bottomLeft;
        QPointF rightTmp = bottomRight;
        bottomLeft = topLeft;
        topLeft = leftTmp;
        bottomRight = topRight;
        topRight = rightTmp;
    }

    QLineF topLine(topLeft, topRight);
    qreal angle = topLine.angle();
    qreal width = topLine.length();

    QLineF leftLine(topLeft, bottomLeft);
    qreal height = leftLine.length();

    int h = rotating()?rotateHeight:height;

    if (mVisible)
    {
        setRect(center.x() - mFrameWidth - width / 2, center.y() - mFrameWidth - mTitleBarHeight - h / 2, width + 2 * mFrameWidth, h + (2 * mFrameWidth) + mTitleBarHeight);
    }
    else
    {
        setRect(center.x() - width / 2, center.y() - h / 2, width, h);
    }

    resetTransform();
    setTransform(QTransform::fromTranslate(center.x(), center.y()), true);
    setTransform(QTransform().rotate(-angle), true);
    setTransform(QTransform::fromTranslate(-center.x(), -center.y()), true);
    //TODO: combine these transforms into one

    mBottomRightResizeGripSvgItem->setParentItem(this);
    mBottomResizeGripSvgItem->setParentItem(this);
    mLeftResizeGripSvgItem->setParentItem(this);
    mRightResizeGripSvgItem->setParentItem(this);
    mTopResizeGripSvgItem->setParentItem(this);
    mRotateButton->setParentItem(this);

    mBottomRightResizeGrip->setParentItem(this);
    mBottomResizeGrip->setParentItem(this);
    mLeftResizeGrip->setParentItem(this);
    mRightResizeGrip->setParentItem(this);
    mTopResizeGrip->setParentItem(this);

    QRectF brRect = mBottomRightResizeGripSvgItem->mapRectToParent(mBottomRightResizeGripSvgItem->boundingRect());
    QRectF bRect = mBottomResizeGripSvgItem->mapRectToParent(mBottomResizeGripSvgItem->boundingRect());
    QRectF lRect = mLeftResizeGripSvgItem->mapRectToParent(mLeftResizeGripSvgItem->boundingRect());
    QRectF rRect = mRightResizeGripSvgItem->mapRectToParent(mRightResizeGripSvgItem->boundingRect());
    QRectF trRect = mTopResizeGripSvgItem->mapRectToParent(mTopResizeGripSvgItem->boundingRect());

    mBottomRightResizeGripSvgItem->setPos(rect().right() - brRect.width(), rect().bottom() - brRect.height());
    mBottomResizeGripSvgItem->setPos(rect().center().x() - bRect.width() / 2, rect().bottom() - bRect.height());

    mLeftResizeGripSvgItem->setPos(rect().left(), rect().center().y() - lRect.height() / 2);
    mRightResizeGripSvgItem->setPos(rect().right() - rRect.width(), rect().center().y() - rRect.height() / 2);

    mTopResizeGripSvgItem->setPos(rect().center().x() - trRect.width() / 2, rect().y());
    mRotateButton->setPos(rect().right() - mFrameWidth - 5, rect().top() + 5);

    mBottomRightResizeGrip->setRect(bottomRightResizeGripRect());
    mBottomResizeGrip->setRect(bottomResizeGripRect());
    mLeftResizeGrip->setRect(leftResizeGripRect());
    mRightResizeGrip->setRect(rightResizeGripRect());
    mTopResizeGrip->setRect(topResizeGripRect());

    QVariant vLocked = delegated()->data(WBGraphicsItemData::ItemLocked);
    bool isLocked = (vLocked.isValid() && vLocked.toBool());
    bool bShowHorizontalResizers = ResizingHorizontally == mOperationMode;
    bool bShowVerticalResizers   = ResizingHorizontally != mOperationMode;
    bool bShowAllResizers        = Resizing == mOperationMode || Scaling == mOperationMode ;

    mBottomRightResizeGripSvgItem->setVisible(!isLocked && bShowAllResizers);
    mBottomResizeGripSvgItem->setVisible(!isLocked && (bShowVerticalResizers || bShowAllResizers));
    mLeftResizeGripSvgItem->setVisible(!isLocked && (bShowHorizontalResizers || bShowAllResizers));
    mRightResizeGripSvgItem->setVisible(!isLocked && (bShowHorizontalResizers || bShowAllResizers));
    mTopResizeGripSvgItem->setVisible(!isLocked && (bShowVerticalResizers || bShowAllResizers));
    mRotateButton->setVisible(mDelegate->testUBFlags(GF_REVOLVABLE) && !isLocked);

    mBottomRightResizeGrip->setVisible(!isLocked && bShowAllResizers);
    mBottomResizeGrip->setVisible(!isLocked && (bShowVerticalResizers || bShowAllResizers));
    mLeftResizeGrip->setVisible(!isLocked && (bShowHorizontalResizers || bShowAllResizers));
    mRightResizeGrip->setVisible(!isLocked && (bShowHorizontalResizers || bShowAllResizers));
    mTopResizeGrip->setVisible(!isLocked && (bShowVerticalResizers || bShowAllResizers));

    if (isLocked)
    {
        QColor baseColor = WBSettings::paletteColor;
        baseColor.setAlphaF(baseColor.alphaF() / 3);
        setBrush(QBrush(baseColor));
    }
    else
    {
        setBrush(QBrush(WBSettings::paletteColor));
    }

    //make frame interact like delegated item when selected. Maybe should be deleted if selection logic will change
    setZValue(delegated()->zValue());
}

QGraphicsItem* WBGraphicsDelegateFrame::delegated()
{
    return mDelegate->delegated();
}

WBGraphicsDelegateFrame::FrameTool WBGraphicsDelegateFrame::toolFromPos(QPointF pos)
{
    if(mDelegate->isLocked())
        return None;
    else if (bottomRightResizeGripRect().contains(pos) && ResizingHorizontally != mOperationMode)
        return ResizeBottomRight;
    else if (bottomResizeGripRect().contains(pos) && ResizingHorizontally != mOperationMode){
            if(mMirrorY){
                return ResizeTop;
            }else{
                return ResizeBottom;
            }
        }
    else if (leftResizeGripRect().contains(pos)){
            if(mMirrorX){
                return ResizeRight;
            }else{
                return ResizeLeft;
            }
            return ResizeLeft;
        }
    else if (rightResizeGripRect().contains(pos)){
            if(mMirrorX){
                return ResizeLeft;
            }else{
                return ResizeRight;
            }
        }
    else if (topResizeGripRect().contains(pos) && ResizingHorizontally != mOperationMode){
            if(mMirrorY){
                return ResizeBottom;
            }else{
                return ResizeTop;
            }
        }
    else if (rotateButtonBounds().contains(pos) && mDelegate && mDelegate->testUBFlags(GF_REVOLVABLE))
        return Rotate;
    else
        return Move;
}

QRectF WBGraphicsDelegateFrame::bottomRightResizeGripRect() const
{
    return QRectF(rect().right() - mFrameWidth, rect().bottom() - mFrameWidth, mFrameWidth, mFrameWidth);
}

QRectF WBGraphicsDelegateFrame::bottomResizeGripRect() const
{
    return QRectF(rect().center().x() - mFrameWidth / 2, rect().bottom() - mFrameWidth, mFrameWidth, mFrameWidth);
}

QRectF WBGraphicsDelegateFrame::leftResizeGripRect() const
{
    return QRectF(rect().left(), rect().center().y() - mFrameWidth / 2, mFrameWidth, mFrameWidth);
}


QRectF WBGraphicsDelegateFrame::rightResizeGripRect() const
{
    return QRectF(rect().right() - mFrameWidth, rect().center().y() - mFrameWidth / 2, mFrameWidth, mFrameWidth);
}

QRectF WBGraphicsDelegateFrame::topResizeGripRect() const
{
    return QRectF(rect().center().x() - mFrameWidth / 2, rect().top(), mFrameWidth, mFrameWidth);
}

QRectF WBGraphicsDelegateFrame::rotateButtonBounds() const
{
    return QRectF(rect().right()- mFrameWidth, rect().top(), mFrameWidth, mFrameWidth);
}

void WBGraphicsDelegateFrame::refreshGeometry()
{
    // Here we want to have the left on the left, the right on the right, the top on the top and the bottom on the bottom!
    QRectF itemRect = delegated()->boundingRect();
    QTransform itemTransform = delegated()->sceneTransform();
    QPointF topLeft = itemTransform.map(itemRect.topLeft());
    QPointF topRight = itemTransform.map(itemRect.topRight());
    QPointF bottomLeft = itemTransform.map(itemRect.bottomLeft());

    QLineF topLine(topLeft, topRight);
    qreal width = topLine.length();
    QLineF leftLine(topLeft, bottomLeft);
    qreal height = leftLine.length();
    setRect(topRight.x() - mFrameWidth, topLeft.y() - mFrameWidth, width + 2*mFrameWidth, height + 2*mFrameWidth);
}
