#include <QPixmap>

#include "tools/WBGraphicsRuler.h"
#include "domain/WBGraphicsScene.h"
#include "core/WBApplication.h"
#include "gui/WBResources.h"
#include "board/WBBoardController.h"
#include "board/WBDrawingController.h"

#include "core/memcheck.h"

const QRect WBGraphicsRuler::sDefaultRect = QRect(0, 0, 800, 96);


WBGraphicsRuler::WBGraphicsRuler()
    : QGraphicsRectItem()
    , mResizing(false)
    , mRotating(false)
{
    setRect(sDefaultRect);

    mResizeSvgItem = new QGraphicsSvgItem(":/images/resizeRuler.svg", this);
    mResizeSvgItem->setVisible(false);
    mResizeSvgItem->setData(WBGraphicsItemData::ItemLayerType, QVariant(WBItemLayerType::Control));

    mRotateSvgItem = new QGraphicsSvgItem(":/images/rotateTool.svg", this);
    mRotateSvgItem->setVisible(false);
    mRotateSvgItem->setData(WBGraphicsItemData::ItemLayerType, QVariant(WBItemLayerType::Control));

    create(*this);

    setData(WBGraphicsItemData::itemLayerType, QVariant(itemLayerType::CppTool)); //Necessary to set if we want z value to be assigned correctly

    setFlag(QGraphicsItem::ItemIsSelectable, false);
    updateResizeCursor();
}

void WBGraphicsRuler::updateResizeCursor()
{
    QPixmap pix(":/images/cursors/resize.png");
    QTransform itemTransform = sceneTransform();
    QRectF itemRect = boundingRect();
    QPointF topLeft = itemTransform.map(itemRect.topLeft());
    QPointF topRight = itemTransform.map(itemRect.topRight());
    QLineF topLine(topLeft, topRight);
    qreal angle = topLine.angle();
    QTransform tr;
    tr.rotate(- angle);
    QCursor resizeCursor  = QCursor(pix.transformed(tr, Qt::SmoothTransformation), pix.width() / 2,  pix.height() / 2);
    mResizeCursor = resizeCursor;
}


WBGraphicsRuler::~WBGraphicsRuler()
{
    // NOOP
}

WBItem* WBGraphicsRuler::deepCopy() const
{
    WBGraphicsRuler* copy = new WBGraphicsRuler();

    copyItemParameters(copy);

    return copy;
}

void WBGraphicsRuler::copyItemParameters(WBItem *copy) const
{
    WBGraphicsRuler *cp = dynamic_cast<WBGraphicsRuler*>(copy);
    if (cp)
    {
        cp->setPos(this->pos());
        cp->setRect(this->rect());
        cp->setTransform(this->transform());
    }
}

void WBGraphicsRuler::paint(QPainter *painter, const QStyleOptionGraphicsItem *styleOption, QWidget *widget)
{
    Q_UNUSED(styleOption);
    Q_UNUSED(widget);

    WBAbstractDrawRuler::paint();

    QTransform antiScaleTransform2;
    qreal ratio = mAntiScaleRatio > 1.0 ? mAntiScaleRatio : 1.0;
    antiScaleTransform2.scale(ratio, 1.0);

    mResizeSvgItem->setTransform(antiScaleTransform2);
    mResizeSvgItem->setPos(resizeButtonRect().topLeft());

    mRotateSvgItem->setTransform(antiScaleTransform2);
    mRotateSvgItem->setPos(rotateButtonRect().topLeft());



    painter->setPen(drawColor());
    painter->setBrush(edgeFillColor());
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->drawRoundedRect(rect(), sRoundingRadius, sRoundingRadius);
    fillBackground(painter);
    paintGraduations(painter);
    if (mRotating)
        paintRotationCenter(painter);
}


QVariant WBGraphicsRuler::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == QGraphicsItem::ItemVisibleHasChanged)
    {
        mCloseSvgItem->setParentItem(this);
        mResizeSvgItem->setParentItem(this);
        mRotateSvgItem->setParentItem(this);
    }

    return QGraphicsRectItem::itemChange(change, value);
}

void WBGraphicsRuler::fillBackground(QPainter *painter)
{
    QRectF rect1(rect().topLeft(), QSizeF(rect().width(), rect().height() / 4));
    QLinearGradient linearGradient1(
        rect1.topLeft(),
        rect1.bottomLeft());
    linearGradient1.setColorAt(0, edgeFillColor());
    linearGradient1.setColorAt(1, middleFillColor());
    painter->fillRect(rect1, linearGradient1);

    QRectF rect2(rect1.bottomLeft(), QSizeF(rect().width(), rect().height() / 2));
    painter->fillRect(rect2, middleFillColor());

    QRectF rect3(rect2.bottomLeft(), rect1.size());
    QLinearGradient linearGradient3(
        rect3.topLeft(),
        rect3.bottomLeft());
    linearGradient3.setColorAt(0, middleFillColor());
    linearGradient3.setColorAt(1, edgeFillColor());
    painter->fillRect(rect3, linearGradient3);
}

void WBGraphicsRuler::paintGraduations(QPainter *painter)
{
    painter->save();
    painter->setFont(font());
    QFontMetricsF fontMetrics(painter->font());

    // Update the width of one "centimeter" to correspond to the width of the background grid (whether it is displayed or not)
    sPixelsPerCentimeter = WBApplication::boardController->activeScene()->backgroundGridSize();

    qreal pixelsPerMillimeter = sPixelsPerCentimeter/10.0;
    int rulerLengthInMillimeters = (rect().width() - sLeftEdgeMargin - sRoundingRadius)/pixelsPerMillimeter;

    // When a "centimeter" is too narrow, we only display every 5th number, and every 5th millimeter mark
    double numbersWidth = fontMetrics.width("00");
    bool shouldDisplayAllNumbers = (numbersWidth <= (sPixelsPerCentimeter - 5));

    for (int millimeters(0); millimeters < rulerLengthInMillimeters; millimeters++) {

        double graduationX = rotationCenter().x() + pixelsPerMillimeter * millimeters;
        double graduationHeight = 0;

        if (millimeters % WBGeometryUtils::millimetersPerCentimeter == 0)
            graduationHeight = WBGeometryUtils::centimeterGraduationHeight;

        else if (millimeters % WBGeometryUtils::millimetersPerHalfCentimeter == 0)
            graduationHeight = WBGeometryUtils::halfCentimeterGraduationHeight;

        else
            graduationHeight = WBGeometryUtils::millimeterGraduationHeight;


        if (shouldDisplayAllNumbers || millimeters % WBGeometryUtils::millimetersPerHalfCentimeter == 0) {
            painter->drawLine(QLineF(graduationX, rotationCenter().y(), graduationX, rotationCenter().y() + graduationHeight));
            painter->drawLine(QLineF(graduationX, rotationCenter().y() + rect().height(), graduationX, rotationCenter().y() + rect().height() - graduationHeight));
        }


        if ((shouldDisplayAllNumbers && millimeters % WBGeometryUtils::millimetersPerCentimeter == 0)
            || millimeters % (WBGeometryUtils::millimetersPerCentimeter*5) == 0)
        {
            QString text = QString("%1").arg((int)(millimeters / WBGeometryUtils::millimetersPerCentimeter));

            if (graduationX + fontMetrics.width(text) / 2 < rect().right()) {
                qreal textWidth = fontMetrics.width(text);
                qreal textHeight = fontMetrics.tightBoundingRect(text).height() + 5;
                painter->drawText(
                    QRectF(graduationX - textWidth / 2, rect().top() + 5 + WBGeometryUtils::centimeterGraduationHeight, textWidth, textHeight),
                    Qt::AlignVCenter, text);
                painter->drawText(
                    QRectF(graduationX - textWidth / 2, rect().bottom() - 5 - WBGeometryUtils::centimeterGraduationHeight - textHeight, textWidth, textHeight),
                    Qt::AlignVCenter, text);
            }
        }
    }


    painter->restore();

}

void WBGraphicsRuler::paintRotationCenter(QPainter *painter)
{
    painter->drawArc(
        rotationCenter().x() - sRotationRadius, rotationCenter().y() - sRotationRadius,
        2 * sRotationRadius, 2 * sRotationRadius,
        270 * sDegreeToQtAngleUnit, 90 * sDegreeToQtAngleUnit);
}

void WBGraphicsRuler::rotateAroundCenter(qreal angle)
{
    QTransform transform;
    transform.translate(rotationCenter().x(), rotationCenter().y());
    transform.rotate(angle);
    transform.translate(- rotationCenter().x(), - rotationCenter().y());
    setTransform(transform, true);
}

QPointF WBGraphicsRuler::rotationCenter() const
{
    return QPointF(rect().x() + sLeftEdgeMargin, rect().y());
}


QRectF WBGraphicsRuler::resizeButtonRect() const
{
    QPixmap resizePixmap(":/images/resizeRuler.svg");
    QSizeF resizeRectSize(
        resizePixmap.rect().width(),
        rect().height());

    qreal ratio = mAntiScaleRatio > 1.0 ? mAntiScaleRatio : 1.0;
    QPointF resizeRectTopLeft(rect().width() - resizeRectSize.width() * ratio, 0);

    QRectF resizeRect(resizeRectTopLeft, resizeRectSize);
    resizeRect.translate(rect().topLeft());

    return resizeRect;
}

QRectF WBGraphicsRuler::closeButtonRect() const
{
    QPixmap closePixmap(":/images/closeTool.svg");

    QSizeF closeRectSize(
        closePixmap.width() * mAntiScaleRatio,
        closePixmap.height() * mAntiScaleRatio);

    QPointF closeRectCenter(
        rect().left() + sLeftEdgeMargin + sPixelsPerCentimeter/2,
        rect().center().y());

    QPointF closeRectTopLeft(
        closeRectCenter.x() - closeRectSize.width() / 2,
        closeRectCenter.y() - closeRectSize.height() / 2);

    return QRectF(closeRectTopLeft, closeRectSize);
}

QRectF WBGraphicsRuler::rotateButtonRect() const
{
    QPixmap rotatePixmap(":/images/closeTool.svg");

    QSizeF rotateRectSize(
        rotatePixmap.width() * mAntiScaleRatio,
        rotatePixmap.height() * mAntiScaleRatio);

    int centimeters = (int)(rect().width() - sLeftEdgeMargin - resizeButtonRect().width()) / (sPixelsPerCentimeter);
    QPointF rotateRectCenter(
        rect().left() + sLeftEdgeMargin + (centimeters - 0.5) * sPixelsPerCentimeter,
        rect().center().y());

    QPointF rotateRectTopLeft(
        rotateRectCenter.x() - rotateRectSize.width() / 2,
        rotateRectCenter.y() - rotateRectSize.height() / 2);

    return QRectF(rotateRectTopLeft, rotateRectSize);
}

void WBGraphicsRuler::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    WBStylusTool::Enum currentTool = (WBStylusTool::Enum)WBDrawingController::drawingController ()->stylusTool ();

    if (currentTool == WBStylusTool::Selector || currentTool == WBStylusTool::Play)
    {
        mCloseSvgItem->setVisible(mShowButtons);
        mResizeSvgItem->setVisible(mShowButtons);
        mRotateSvgItem->setVisible(mShowButtons);
        if (resizeButtonRect().contains(event->pos()))
            setCursor(resizeCursor());
        else if (closeButtonRect().contains(event->pos()))
            setCursor(closeCursor());
        else if (rotateButtonRect().contains(event->pos()))
            setCursor(rotateCursor());
        else
            setCursor(moveCursor());

        event->accept();
    }
    else if (WBDrawingController::drawingController()->isDrawingTool())
    {
        event->accept();
    }
}

void WBGraphicsRuler::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->pos().x() > resizeButtonRect().left())
    {
        mResizing = true;
        event->accept();
    }
    else if (rotateButtonRect().contains(event->pos()))
    {
        mRotating = true;
        event->accept();
    }
    else
    {
        mResizeSvgItem->setVisible(false);
        mRotateSvgItem->setVisible(false);
        QGraphicsItem::mousePressEvent(event);
    }
    mResizeSvgItem->setVisible(mShowButtons && mResizing);
    mRotateSvgItem->setVisible(mShowButtons && mRotating);
    mCloseSvgItem->setVisible(false);
}

void WBGraphicsRuler::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (!mResizing && !mRotating)
    {
        QGraphicsItem::mouseMoveEvent(event);
    }
    else
    {
        if (mResizing)
        {
            QPointF delta = event->pos() - event->lastPos();
            if (rect().width() + delta.x() < sMinLength)
                delta.setX(sMinLength - rect().width());

            if (rect().width() + delta.x() > sMaxLength)
                delta.setX(sMaxLength - rect().width());

            setRect(QRectF(rect().topLeft(), QSizeF(rect().width() + delta.x(), rect().height())));
        }
        else
        {
            QLineF currentLine(rotationCenter(), event->pos());
            QLineF lastLine(rotationCenter(), event->lastPos());
            rotateAroundCenter(currentLine.angleTo(lastLine));
        }

        event->accept();
    }
}

void WBGraphicsRuler::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (mResizing)
    {
        mResizing = false;
        event->accept();
    }
    else if (mRotating)
    {
        mRotating = false;
        updateResizeCursor();
        update(QRectF(rotationCenter(), QSizeF(sRotationRadius, sRotationRadius)));
        event->accept();
    }
    else if (closeButtonRect().contains(event->pos()))
    {
        hide();
        event->accept();
    }
    else
    {
        QGraphicsItem::mouseReleaseEvent(event);
    }

    if (scene())
        scene()->setModified(true);
}

void WBGraphicsRuler::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    WBStylusTool::Enum currentTool = (WBStylusTool::Enum)WBDrawingController::drawingController ()->stylusTool ();

    if (currentTool == WBStylusTool::Selector ||
        currentTool == WBStylusTool::Play)
    {
        mCloseSvgItem->setParentItem(this);
        mResizeSvgItem->setParentItem(this);
        mRotateSvgItem->setParentItem(this);

        mShowButtons = true;
        mCloseSvgItem->setVisible(mShowButtons);
        mResizeSvgItem->setVisible(mShowButtons);
        mRotateSvgItem->setVisible(mShowButtons);
        if (event->pos().x() >= resizeButtonRect().left())
        {
            setCursor(resizeCursor());
        }
        else
        {
            if (closeButtonRect().contains(event->pos()))
                setCursor(closeCursor());
            else if (rotateButtonRect().contains(event->pos()))
                setCursor(rotateCursor());
            else
                setCursor(moveCursor());
        }
        event->accept();
        update();
    }
    else if (WBDrawingController::drawingController()->isDrawingTool())
    {
        setCursor(drawRulerLineCursor());
        WBDrawingController::drawingController()->mActiveRuler = this;
        event->accept();
    }
}

void WBGraphicsRuler::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    mShowButtons = false;
    setCursor(Qt::ArrowCursor);
    mCloseSvgItem->setVisible(mShowButtons);
    mResizeSvgItem->setVisible(mShowButtons);
    mRotateSvgItem->setVisible(mShowButtons);
    WBDrawingController::drawingController()->mActiveRuler = NULL;
    event->accept();
    update();
}



WBGraphicsScene* WBGraphicsRuler::scene() const
{
    return static_cast<WBGraphicsScene*>(QGraphicsRectItem::scene());
}

void WBGraphicsRuler::StartLine(const QPointF& scenePos, qreal width)
{
    Q_UNUSED(width);

    QPointF itemPos = mapFromScene(scenePos);

    mStrokeWidth = WBDrawingController::drawingController()->currentToolWidth();

    qreal y;

    if (itemPos.y() > rect().y() + rect().height() / 2)
    {
        drawLineDirection = 0;
        y = rect().y() + rect().height() + mStrokeWidth / 2;
    }
    else
    {
        drawLineDirection = 1;
        y = rect().y() - mStrokeWidth /2;
    }

    if (itemPos.x() < rect().x() + sLeftEdgeMargin)
        itemPos.setX(rect().x() + sLeftEdgeMargin);
    if (itemPos.x() > rect().x() + rect().width() - sLeftEdgeMargin)
        itemPos.setX(rect().x() + rect().width() - sLeftEdgeMargin);

    itemPos.setY(y);
    itemPos = mapToScene(itemPos);

    scene()->moveTo(itemPos);
    scene()->drawLineTo(itemPos, mStrokeWidth, true);
}

void WBGraphicsRuler::DrawLine(const QPointF& scenePos, qreal width)
{
    Q_UNUSED(width);
    QPointF itemPos = mapFromScene(scenePos);

    qreal y;
    if (drawLineDirection == 0)
    {
        y = rect().y() + rect().height() + mStrokeWidth / 2;
    }
    else
    {
        y = rect().y() - mStrokeWidth /2;
    }
    if (itemPos.x() < rect().x() + sLeftEdgeMargin)
        itemPos.setX(rect().x() + sLeftEdgeMargin);
    if (itemPos.x() > rect().x() + rect().width() - sLeftEdgeMargin)
        itemPos.setX(rect().x() + rect().width() - sLeftEdgeMargin);

    itemPos.setY(y);
    itemPos = mapToScene(itemPos);

    // We have to use "pointed" line for marker tool
    scene()->drawLineTo(itemPos, mStrokeWidth, WBDrawingController::drawingController()->stylusTool() != WBStylusTool::Marker);
}

void WBGraphicsRuler::EndLine()
{
    // We never come to this place
    scene()->inputDeviceRelease();
}
