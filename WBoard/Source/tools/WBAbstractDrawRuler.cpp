#include "WBAbstractDrawRuler.h"
#include <QtSvg>
#include "core/WB.h"
#include "gui/WBResources.h"
#include "domain/WBGraphicsScene.h"
#include "board/WBDrawingController.h"
#include "core/WBApplication.h"
#include "board/WBBoardController.h"

#include "core/memcheck.h"


const QColor WBAbstractDrawRuler::sLightBackgroundMiddleFillColor = QColor(0x72, 0x72, 0x72, sFillTransparency);
const QColor WBAbstractDrawRuler::sLightBackgroundEdgeFillColor = QColor(0xc3, 0xc3, 0xc3, sFillTransparency);
const QColor WBAbstractDrawRuler::sLightBackgroundDrawColor = QColor(0x33, 0x33, 0x33, sDrawTransparency);
const QColor WBAbstractDrawRuler::sDarkBackgroundMiddleFillColor = QColor(0xb5, 0xb5, 0xb5, sFillTransparency);
const QColor WBAbstractDrawRuler::sDarkBackgroundEdgeFillColor = QColor(0xdd, 0xdd, 0xdd, sFillTransparency);
const QColor WBAbstractDrawRuler::sDarkBackgroundDrawColor = QColor(0xff, 0xff, 0xff, sDrawTransparency);

const int WBAbstractDrawRuler::sLeftEdgeMargin = 10;
const int WBAbstractDrawRuler::sDegreeToQtAngleUnit = 16;
const int WBAbstractDrawRuler::sRotationRadius = 15;
const int WBAbstractDrawRuler::sFillTransparency = 127;
const int WBAbstractDrawRuler::sDrawTransparency = 192;
const int WBAbstractDrawRuler::sRoundingRadius = sLeftEdgeMargin / 2;


WBAbstractDrawRuler::WBAbstractDrawRuler()
    : mShowButtons(false)
    , mAntiScaleRatio(1.0)
{
    sPixelsPerCentimeter = WBApplication::boardController->activeScene()->backgroundGridSize();
}

void WBAbstractDrawRuler::create(QGraphicsItem& item)
{
    item.setFlag(QGraphicsItem::ItemIsMovable, true);
    item.setFlag(QGraphicsItem::ItemIsSelectable, true);
    item.setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    item.setAcceptHoverEvents(true);

    mCloseSvgItem = new QGraphicsSvgItem(":/images/closeTool.svg", &item);
    mCloseSvgItem->setVisible(false);
    mCloseSvgItem->setData(WBGraphicsItemData::ItemLayerType, QVariant(WBItemLayerType::Control));
}


WBAbstractDrawRuler::~WBAbstractDrawRuler()
{
}

QCursor WBAbstractDrawRuler::moveCursor() const
{
    return Qt::SizeAllCursor;
}

QCursor WBAbstractDrawRuler::rotateCursor() const
{
    return WBResources::resources()->rotateCursor;
}

QCursor WBAbstractDrawRuler::closeCursor() const
{
    return Qt::ArrowCursor;
}

QCursor WBAbstractDrawRuler::drawRulerLineCursor() const
{
    return WBResources::resources()->drawLineRulerCursor;
}

QColor WBAbstractDrawRuler::drawColor() const
{
    return scene()->isDarkBackground() ? sDarkBackgroundDrawColor : sLightBackgroundDrawColor;
}

QColor WBAbstractDrawRuler::middleFillColor() const
{
    return scene()->isDarkBackground() ? sDarkBackgroundMiddleFillColor : sLightBackgroundMiddleFillColor;
}

QColor WBAbstractDrawRuler::edgeFillColor() const
{
    return scene()->isDarkBackground() ? sDarkBackgroundEdgeFillColor : sLightBackgroundEdgeFillColor;
}

QFont WBAbstractDrawRuler::font() const
{
    QFont font("Arial");
    font.setPixelSize(16);
    return font;
}

void WBAbstractDrawRuler::StartLine(const QPointF& position, qreal width)
{
    Q_UNUSED(position);
    Q_UNUSED(width);
}
void WBAbstractDrawRuler::DrawLine(const QPointF& position, qreal width)
{
    Q_UNUSED(position);
    Q_UNUSED(width);
}
void WBAbstractDrawRuler::EndLine()
{}


void WBAbstractDrawRuler::paint()
{
    mAntiScaleRatio = 1 / (WBApplication::boardController->systemScaleFactor() * WBApplication::boardController->currentZoom());
    QTransform antiScaleTransform;
    antiScaleTransform.scale(mAntiScaleRatio, mAntiScaleRatio);

    mCloseSvgItem->setTransform(antiScaleTransform);
    mCloseSvgItem->setPos(closeButtonRect().topLeft());

}


