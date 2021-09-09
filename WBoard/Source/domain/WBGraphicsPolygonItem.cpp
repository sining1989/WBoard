#include "WBGraphicsPolygonItem.h"

#include "frameworks/WBGeometryUtils.h"
#include "WBGraphicsScene.h"
#include "domain/WBGraphicsPolygonItem.h"
#include "domain/WBGraphicsStroke.h"

#include "core/memcheck.h"

WBGraphicsPolygonItem::WBGraphicsPolygonItem (QGraphicsItem * parent)
    : QGraphicsPolygonItem(parent)
    , mHasAlpha(false)
    , mOriginalWidth(-1)
    , mIsNominalLine(false)
    , mStroke(0)
    , mpGroup(NULL)
{
    // NOOP
    initialize();
}


WBGraphicsPolygonItem::WBGraphicsPolygonItem (const QPolygonF & polygon, QGraphicsItem * parent)
    : QGraphicsPolygonItem(polygon, parent)
    , mOriginalWidth(-1)
    , mIsNominalLine(false)
    , mStroke(0)
    , mpGroup(NULL)
{
    // NOOP
    initialize();
}


WBGraphicsPolygonItem::WBGraphicsPolygonItem (const QLineF& pLine, qreal pWidth)
    : QGraphicsPolygonItem(WBGeometryUtils::lineToPolygon(pLine, pWidth))
    , mOriginalLine(pLine)
    , mOriginalWidth(pWidth)
    , mIsNominalLine(true)
    , mStroke(0)
    , mpGroup(NULL)
{
    // NOOP
    initialize();
}

WBGraphicsPolygonItem::WBGraphicsPolygonItem (const QLineF& pLine, qreal pStartWidth, qreal pEndWidth)
    : QGraphicsPolygonItem(WBGeometryUtils::lineToPolygon(pLine, pStartWidth, pEndWidth))
    , mOriginalLine(pLine)
    , mOriginalWidth(pEndWidth)
    , mIsNominalLine(true)
    , mStroke(0)
    , mpGroup(NULL)
{
    // NOOP
    initialize();
}


void WBGraphicsPolygonItem::initialize()
{
    setData(WBGraphicsItemData::itemLayerType, QVariant(itemLayerType::DrawingItem)); //Necessary to set if we want z value to be assigned correctly
    setUuid(QUuid::createUuid());
}

void WBGraphicsPolygonItem::setUuid(const QUuid &pUuid)
{
    WBItem::setUuid(pUuid);
    setData(WBGraphicsItemData::ItemUuid, QVariant(pUuid)); //store item uuid inside the QGraphicsItem to fast operations with Items on the scene
}

void WBGraphicsPolygonItem::clearStroke()
{
    if (mStroke!=NULL)
    {
        mStroke->remove(this);
        if (mStroke->polygons().empty())
            delete mStroke;
        mStroke = NULL;
    }
}

WBGraphicsPolygonItem::~WBGraphicsPolygonItem()
{
    clearStroke();
}

void WBGraphicsPolygonItem::setStrokesGroup(WBGraphicsStrokesGroup *group)
{
    mpGroup = group;
}

void WBGraphicsPolygonItem::setStroke(WBGraphicsStroke* stroke)
{
    if (stroke) {
        clearStroke();

        mStroke = stroke;
        mStroke->addPolygon(this);
    }
}

WBGraphicsStroke* WBGraphicsPolygonItem::stroke() const
{
    return mStroke;
}


void WBGraphicsPolygonItem::setColor(const QColor& pColor)
{
    QGraphicsPolygonItem::setBrush(QBrush(pColor));
    setPen(Qt::NoPen);

    mHasAlpha = (pColor.alphaF() < 1.0);
}


QColor WBGraphicsPolygonItem::color() const
{
    return QGraphicsPolygonItem::brush().color();
}


WBItem* WBGraphicsPolygonItem::deepCopy() const
{
    WBGraphicsPolygonItem* copy = new WBGraphicsPolygonItem(polygon(), 0);
    copyItemParameters(copy);
    return copy;
}


void WBGraphicsPolygonItem::copyItemParameters(WBItem *copy) const
{
    WBGraphicsPolygonItem *cp = dynamic_cast<WBGraphicsPolygonItem*>(copy);
    if (cp)
    {
        cp->mOriginalLine = this->mOriginalLine;
        cp->mOriginalWidth = this->mOriginalWidth;
        cp->mIsNominalLine = this->mIsNominalLine;

        cp->setTransform(transform());
        cp->setBrush(this->brush());
        cp->setPen(this->pen());
        cp->mHasAlpha = this->mHasAlpha;
        cp->setFillRule(this->fillRule());

        cp->setColorOnDarkBackground(this->colorOnDarkBackground());
        cp->setColorOnLightBackground(this->colorOnLightBackground());

        cp->setZValue(this->zValue());
        cp->setData(WBGraphicsItemData::ItemLayerType, this->data(WBGraphicsItemData::ItemLayerType));
    }
}

void WBGraphicsPolygonItem::paint ( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
    if(mHasAlpha && scene() && scene()->isLightBackground())
        painter->setCompositionMode(QPainter::CompositionMode_SourceOver);

    painter->setRenderHints(QPainter::Antialiasing);

    QGraphicsPolygonItem::paint(painter, option, widget);
}

WBGraphicsScene* WBGraphicsPolygonItem::scene()
{
    return qobject_cast<WBGraphicsScene*>(QGraphicsPolygonItem::scene());
}
