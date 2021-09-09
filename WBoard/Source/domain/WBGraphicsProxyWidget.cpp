#include "WBGraphicsProxyWidget.h"

#include <QtWidgets>

#include "WBGraphicsScene.h"
#include "WBGraphicsItemDelegate.h"

#include "WBGraphicsDelegateFrame.h"

#include "core/WBApplication.h"
#include "core/WBPersistenceManager.h"

#include "board/WBBoardController.h"

#include "frameworks/WBFileSystemUtils.h"

#include "core/memcheck.h"

WBGraphicsProxyWidget::WBGraphicsProxyWidget(QGraphicsItem* parent)
    : QGraphicsProxyWidget(parent, Qt::FramelessWindowHint)
{
    setData(WBGraphicsItemData::ItemLayerType, WBItemLayerType::Object);

    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    QGraphicsProxyWidget::setAcceptHoverEvents(true);
}


WBGraphicsProxyWidget::~WBGraphicsProxyWidget()
{
}

void WBGraphicsProxyWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->save();
    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    QGraphicsProxyWidget::paint(painter,option,widget);
    Delegate()->postpaint(painter, option, widget);
    painter->restore();
}

QVariant WBGraphicsProxyWidget::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == QGraphicsItem::ItemCursorHasChanged &&  scene())
    {
        unsetCursor();
    }
    if ((change == QGraphicsItem::ItemSelectedHasChanged)
              &&  scene())
    {
        if (isSelected())
        {
            scene()->setActiveWindow(this);
        }
        else
        {
            if(scene()->activeWindow() == this)
            {
                scene()->setActiveWindow(0);
            }
        }
    }

    if (Delegate()) {
        QVariant newValue = Delegate()->itemChange(change, value);
        return QGraphicsProxyWidget::itemChange(change, newValue);
    }
    else
        return QGraphicsProxyWidget::itemChange(change, value);
}

void WBGraphicsProxyWidget::setUuid(const QUuid &pUuid)
{
    WBItem::setUuid(pUuid);
    setData(WBGraphicsItemData::ItemUuid, QVariant(pUuid)); //store item uuid inside the QGraphicsItem to fast operations with Items on the scene
}

void WBGraphicsProxyWidget::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (Delegate()->mousePressEvent(event))
    {
        //NOOP
    }
    else
    {
        // QT Proxy Widget is a bit lazy, we force the selection ...

        setSelected(true);
    }
    QGraphicsProxyWidget::mousePressEvent(event);
}


void WBGraphicsProxyWidget::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (Delegate()->mouseMoveEvent(event))
    {
        // NOOP;
    }
    else
    {
        QGraphicsProxyWidget::mouseMoveEvent(event);
    }
}


void WBGraphicsProxyWidget::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if(Delegate()->mouseReleaseEvent(event)){

    }
    else
        QGraphicsProxyWidget::mouseReleaseEvent(event);
}

void WBGraphicsProxyWidget::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    if( Delegate()->wheelEvent(event) )
    {
        QGraphicsProxyWidget::wheelEvent(event);
        event->accept();
    }
}

void WBGraphicsProxyWidget::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event)
//    NOOP
}
void WBGraphicsProxyWidget::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event)
//    NOOP
}

void WBGraphicsProxyWidget::resize(qreal w, qreal h)
{
    WBGraphicsProxyWidget::resize(QSizeF(w, h));
}


void WBGraphicsProxyWidget::resize(const QSizeF & pSize)
{
    if (pSize != size())
    {
        qreal sizeX = 0;
        qreal sizeY = 0;

        if (widget())
        {

            QSizeF minimumItemSize(widget()->minimumSize());
            if (minimumItemSize.width() > pSize.width())
                sizeX = minimumItemSize.width();
            else
                sizeX = pSize.width();

            if (minimumItemSize.height() > pSize.height())
                sizeY = minimumItemSize.height();
            else
                sizeY = pSize.height();
        }
        QSizeF size(sizeX, sizeY);


        QGraphicsProxyWidget::setMaximumSize(size.width(), size.height());
        QGraphicsProxyWidget::resize(size.width(), size.height());
        if (widget())
            widget()->resize(size.width(), size.height());
        if (Delegate())
            Delegate()->positionHandles();
        if (scene())
            scene()->setModified(true);
    }
}


QSizeF WBGraphicsProxyWidget::size() const
{
    return QGraphicsProxyWidget::size();
}


WBGraphicsScene* WBGraphicsProxyWidget::scene()
{
    return static_cast<WBGraphicsScene*>(QGraphicsItem::scene());
}
