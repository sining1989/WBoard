#include <QtWidgets>
#include "WBGraphicsGroupContainerItem.h"
#include "WBGraphicsTextItem.h"
#include "WBGraphicsTextItemDelegate.h"
#include "WBGraphicsScene.h"
#include "WBGraphicsDelegateFrame.h"

#include "core/WBApplication.h"
#include "board/WBBoardController.h"
#include "board/WBBoardView.h"
#include "board/WBDrawingController.h"
#include "core/WBSettings.h"

#include "core/memcheck.h"
QColor WBGraphicsTextItem::lastUsedTextColor = QColor(Qt::black);

WBGraphicsTextItem::WBGraphicsTextItem(QGraphicsItem * parent)
    : QGraphicsTextItem(parent)
    , WBGraphicsItem()
    , mTypeTextHereLabel(tr("<Type Text Here>"))
    , mMultiClickState(0)
    , mLastMousePressTime(QTime::currentTime())
    , isActivatedTextEditor(true)
{
    setDelegate(new WBGraphicsTextItemDelegate(this, 0));

    // TODO claudio remove this because in contrast with the fact the frame should be created on demand.
    Delegate()->createControls();
    Delegate()->frame()->setOperationMode(WBGraphicsDelegateFrame::Resizing);
    Delegate()->setUBFlag(GF_FLIPPABLE_ALL_AXIS, false);
    Delegate()->setUBFlag(GF_REVOLVABLE, true);

    setData(WBGraphicsItemData::ItemLayerType, WBItemLayerType::Object);
    setData(WBGraphicsItemData::itemLayerType, QVariant(itemLayerType::ObjectItem)); //Necessary to set if we want z value to be assigned correctly

    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    setTextInteractionFlags(Qt::TextEditorInteraction);

    setUuid(QUuid::createUuid());

    connect(document(), SIGNAL(contentsChanged()), Delegate(), SLOT(contentsChanged()));
    connect(document(), SIGNAL(undoCommandAdded()), this, SLOT(undoCommandAdded()));

    connect(document()->documentLayout(), SIGNAL(documentSizeChanged(const QSizeF &)),
            this, SLOT(documentSizeChanged(const QSizeF &)));

}

WBGraphicsTextItem::~WBGraphicsTextItem()
{
}

void WBGraphicsTextItem::recolor()
{
    WBGraphicsTextItemDelegate * del = dynamic_cast<WBGraphicsTextItemDelegate*>(Delegate());
    if (del)
        del->recolor();
}

void WBGraphicsTextItem::setSelected(bool selected)
{
    if(selected){
        Delegate()->createControls();
        Delegate()->frame()->setOperationMode(WBGraphicsDelegateFrame::Resizing);
    }
    QGraphicsTextItem::setSelected(selected);
}

QVariant WBGraphicsTextItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    QVariant newValue = value;

    if(Delegate())
        newValue = Delegate()->itemChange(change, value);

    return QGraphicsTextItem::itemChange(change, newValue);
}

void WBGraphicsTextItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{

    // scene()->itemAt(pos) returns 0 if pos is not over text, but over text item, but mouse press comes.
    // It is a cludge...
    if (WBStylusTool::Play == WBDrawingController::drawingController()->stylusTool())
    {
        QGraphicsTextItem::mousePressEvent(event);
        event->accept();
        clearFocus();
        return;
    }

    if (Delegate())
    {
        Delegate()->mousePressEvent(event);
        if (Delegate() && parentItem() && WBGraphicsGroupContainerItem::Type == parentItem()->type())
        {
            WBGraphicsGroupContainerItem *group = qgraphicsitem_cast<WBGraphicsGroupContainerItem*>(parentItem());
            if (group)
            {
                QGraphicsItem *curItem = group->getCurrentItem();
                if (curItem && this != curItem)
                {
                    group->deselectCurrentItem();
                }
                group->setCurrentItem(this);
                this->setSelected(true);
                Delegate()->positionHandles();
            }

        }
    }

    if (!data(WBGraphicsItemData::ItemEditable).toBool()) {
        setTextInteractionFlags(Qt::NoTextInteraction);
        return;
    }

    setTextInteractionFlags(Qt::TextEditorInteraction);

    int elapsed = mLastMousePressTime.msecsTo(QTime::currentTime());

    if (elapsed < WBApplication::app()->doubleClickInterval())
    {
        mMultiClickState++;
        if (mMultiClickState > 3)
            mMultiClickState = 1;
    }
    else
    {
        mMultiClickState = 1;
    }

    mLastMousePressTime = QTime::currentTime();

    if (mMultiClickState == 1)
    {
        activateTextEditor(true);
        QGraphicsTextItem::mousePressEvent(event);
        setFocus();
    }
    else if (mMultiClickState == 2)
    {
        QTextCursor tc= textCursor();
        tc.select(QTextCursor::WordUnderCursor);
        setTextCursor(tc);
    }
    else if (mMultiClickState == 3)
    {
        QTextCursor tc= textCursor();
        tc.select(QTextCursor::Document);
        setTextCursor(tc);
    }
    else
    {
        mMultiClickState = 0;
    }
}

void WBGraphicsTextItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (!Delegate() || !Delegate()->mouseMoveEvent(event))
    {
        QGraphicsTextItem::mouseMoveEvent(event);
    }
}

void WBGraphicsTextItem::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
{

}

void WBGraphicsTextItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    // scene()->itemAt(pos) returns 0 if pos is not over text, but over text item, but mouse press comes.
    // It is a cludge...
    if (WBStylusTool::Play == WBDrawingController::drawingController()->stylusTool())
    {
        event->accept();
        clearFocus();
        return;
    }

    if (mMultiClickState == 1)
    {
        QGraphicsTextItem::mouseReleaseEvent(event);

        if (Delegate())
            Delegate()->mouseReleaseEvent(event);
    }
    else
    {
        event->accept();
    }
}

void WBGraphicsTextItem::keyPressEvent(QKeyEvent *event)
{
    if (Delegate() && !Delegate()->keyPressEvent(event)) {
        qDebug() << "WBGraphicsTextItem::keyPressEvent(QKeyEvent *event) has been rejected by delegate. Don't call base class method";
        return;
    }

    QGraphicsTextItem::keyPressEvent(event);
}

void WBGraphicsTextItem::keyReleaseEvent(QKeyEvent *event)
{
    if (Delegate() && !Delegate()->keyReleaseEvent(event)) {
        qDebug() << "WBGraphicsTextItem::keyPressEvent(QKeyEvent *event) has been rejected by delegate. Don't call base class method";
        return;
    }

    QGraphicsTextItem::keyReleaseEvent(event);
}

void WBGraphicsTextItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // Never draw the rubber band, we draw our custom selection with the DelegateFrame
    QStyleOptionGraphicsItem styleOption = QStyleOptionGraphicsItem(*option);
    styleOption.state &= ~QStyle::State_Selected;
    styleOption.state &= ~QStyle::State_HasFocus;

    QGraphicsTextItem::paint(painter, &styleOption, widget);

    if (widget == WBApplication::boardController->controlView()->viewport() && !isSelected())
    {
        setTextInteractionFlags(Qt::NoTextInteraction);
        if (toPlainText().isEmpty())
        {
            painter->setFont(font());
            painter->setPen(WBSettings::paletteColor);
            painter->drawText(boundingRect(), Qt::AlignCenter, mTypeTextHereLabel);
        }
    }

    Delegate()->postpaint(painter, option, widget);
}


WBItem* WBGraphicsTextItem::deepCopy() const
{
    WBGraphicsTextItem* copy = new WBGraphicsTextItem();

    copyItemParameters(copy);

   return copy;
}

void WBGraphicsTextItem::copyItemParameters(WBItem *copy) const
{
    WBGraphicsTextItem *cp = dynamic_cast<WBGraphicsTextItem*>(copy);
    if (cp)
    {
        cp->setHtml(toHtml());
        cp->setPos(this->pos());
        cp->setTransform(this->transform());
        cp->setFlag(QGraphicsItem::ItemIsMovable, true);
        cp->setFlag(QGraphicsItem::ItemIsSelectable, true);
        cp->setData(WBGraphicsItemData::ItemLayerType, this->data(WBGraphicsItemData::ItemLayerType));
        cp->setData(WBGraphicsItemData::ItemLocked, this->data(WBGraphicsItemData::ItemLocked));
        cp->setData(WBGraphicsItemData::ItemEditable, data(WBGraphicsItemData::ItemEditable).toBool());
        cp->setTextWidth(this->textWidth());
        cp->setTextHeight(this->textHeight());

        cp->setSourceUrl(this->sourceUrl());
        cp->setZValue(this->zValue());
    }
}

QRectF WBGraphicsTextItem::boundingRect() const
{
    qreal width = textWidth();
    qreal height = textHeight();
    return QRectF(QPointF(), QSizeF(width, height));
}


QPainterPath WBGraphicsTextItem::shape() const
{
    QPainterPath path;
    path.addRect(boundingRect());
    return path;
}

void WBGraphicsTextItem::setTextWidth(qreal width)
{
    qreal titleBarWidth = 0;
    WBGraphicsTextItemDelegate * del = dynamic_cast<WBGraphicsTextItemDelegate*>(Delegate());
    if (del)
        titleBarWidth = del->titleBarWidth();

    qreal newWidth = qMax(titleBarWidth, width);

    QGraphicsTextItem::setTextWidth(newWidth);
}


void WBGraphicsTextItem::setTextHeight(qreal height)
{
    QFontMetrics fm(font());
    qreal minHeight = fm.height() + document()->documentMargin() * 2;
    mTextHeight = qMax(minHeight, height);
    update();
    setFocus();
}


qreal WBGraphicsTextItem::textHeight() const
{
    return mTextHeight;
}

/**
 * @brief Get the ratio between font size in pixels and points.
 * @return The ratio of pixel size to point size of the first character, or 0 if the text item is empty.
 *
 * Qt may display fonts differently on different platforms -- on the same display,
 * the same point size may be displayed at different pixel sizes. This function returns the
 * ratio of pixel size to point size, based on the first character in the text item.
 */
qreal WBGraphicsTextItem::pixelsPerPoint() const
{
    QTextCursor cursor = textCursor();
    if (cursor.isNull())
        return 0;

    cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);

    QFont f = cursor.charFormat().font();
    qDebug() << "ppp. Font: " << f;
    QFontInfo fi(cursor.charFormat().font());

    qreal pixelSize = fi.pixelSize();
    qreal pointSize = fi.pointSizeF();

    //qDebug() << "Pixel size: " << pixelSize;
    //qDebug() << "Point size: " << pointSize;

    if (pointSize == 0)
        return 0;

    return pixelSize/pointSize;
}


void WBGraphicsTextItem::contentsChanged()
{
    if (scene())
    {
        scene()->setModified(true);
    }

    if (toPlainText().isEmpty())
    {
        resize(QFontMetrics(font()).width(mTypeTextHereLabel),QFontMetrics(font()).height());
    }
}


WBGraphicsScene* WBGraphicsTextItem::scene()
{
    return static_cast<WBGraphicsScene*>(QGraphicsItem::scene());
}


void WBGraphicsTextItem::resize(qreal w, qreal h)
{
    setTextWidth(w);
    setTextHeight(h);
    if (Delegate())
        Delegate()->positionHandles();
}


QSizeF WBGraphicsTextItem::size() const
{
    return QSizeF(textWidth(), textHeight());
}

void WBGraphicsTextItem::setUuid(const QUuid &pUuid)
{
    WBItem::setUuid(pUuid);
    setData(WBGraphicsItemData::ItemUuid, QVariant(pUuid)); //store item uuid inside the QGraphicsItem to fast operations with Items on the scene
}


void WBGraphicsTextItem::undoCommandAdded()
{
    emit textUndoCommandAdded(this);
}


void WBGraphicsTextItem::documentSizeChanged(const QSizeF & newSize)
{
    resize(newSize.width(), newSize.height());
}

void WBGraphicsTextItem::activateTextEditor(bool activate)
{
    qDebug() << textInteractionFlags();

    this->isActivatedTextEditor = activate;

    if(!activate){
        setTextInteractionFlags(Qt::TextSelectableByMouse);
    }else{
        setTextInteractionFlags(Qt::TextEditorInteraction);
    }

    qDebug() <<  textInteractionFlags();
}
