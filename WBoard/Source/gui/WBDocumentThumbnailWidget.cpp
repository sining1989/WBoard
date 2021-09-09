#include "WBDocumentThumbnailWidget.h"

#include "core/WBApplication.h"
#include "core/WBMimeData.h"
#include "core/WBSettings.h"

#include "board/WBBoardController.h"

#include "document/WBDocumentController.h"

#include "core/memcheck.h"


WBDocumentThumbnailWidget::WBDocumentThumbnailWidget(QWidget* parent)
    : WBThumbnailWidget(parent)
    , mDropCaretRectItem(0)
    , mClosestDropItem(0)
    , mDragEnabled(true)
    , mScrollMagnitude(0)
{
    bCanDrag = false;
    mScrollTimer = new QTimer(this);
    connect(mScrollTimer, SIGNAL(timeout()), this, SLOT(autoScroll()));
}


WBDocumentThumbnailWidget::~WBDocumentThumbnailWidget()
{
    // NOOP
}


void WBDocumentThumbnailWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!dragEnabled())
    {
        event->ignore();
        return;
    }

    if (!(event->buttons() & Qt::LeftButton))
        return;

    if ((event->pos() - mMousePressPos).manhattanLength() < QApplication::startDragDistance())
        return;

    QList<QGraphicsItem*> graphicsItems = items(mMousePressPos);

    WBSceneThumbnailPixmap* sceneItem = 0;

    while (!graphicsItems.isEmpty() && !sceneItem)
        sceneItem = dynamic_cast<WBSceneThumbnailPixmap*>(graphicsItems.takeFirst());

    if (sceneItem)
    {
        QDrag *drag = new QDrag(this);
        QList<WBMimeDataItem> mimeDataItems;
        foreach (QGraphicsItem *item, selectedItems())
            mimeDataItems.append(WBMimeDataItem(sceneItem->proxy(), mGraphicItems.indexOf(item)));

        WBMimeData *mime = new WBMimeData(mimeDataItems);
        drag->setMimeData(mime);

        drag->setPixmap(sceneItem->pixmap().scaledToWidth(100));
        drag->setHotSpot(QPoint(drag->pixmap().width()/2, drag->pixmap().height() / 2));

        drag->exec(Qt::MoveAction);
    }

    WBThumbnailWidget::mouseMoveEvent(event);
}

void WBDocumentThumbnailWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (!event->mimeData()->hasFormat(WBApplication::mimeTypeUniboardPage))
    {
        event->setDropAction(Qt::IgnoreAction);
        event->ignore();
        return;
    }

    WBThumbnailWidget::dragEnterEvent(event);
}

void WBDocumentThumbnailWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event);
    if (mScrollTimer->isActive())
    {
        mScrollMagnitude = 0;
        mScrollTimer->stop();
    }
    deleteDropCaret();
    WBThumbnailWidget::dragLeaveEvent(event);
}

void WBDocumentThumbnailWidget::autoScroll()
{
    this->verticalScrollBar()->setValue(this->verticalScrollBar()->value() + mScrollMagnitude);
}

void WBDocumentThumbnailWidget::dragMoveEvent(QDragMoveEvent *event)
{
    QRect boundingFrame = frameRect();
    //setting up automatic scrolling
    const int SCROLL_DISTANCE = 16;
    int bottomDist = boundingFrame.bottom() - event->pos().y(), topDist = boundingFrame.top() - event->pos().y();
    if(qAbs(bottomDist) <= SCROLL_DISTANCE)
    {
        mScrollMagnitude = (SCROLL_DISTANCE - bottomDist)*4;
        if(verticalScrollBar()->isVisible() && !mScrollTimer->isActive()) mScrollTimer->start(100);
    }
    else if(qAbs(topDist) <= SCROLL_DISTANCE)
    {
        mScrollMagnitude = (- SCROLL_DISTANCE - topDist)*4;
        if(verticalScrollBar()->isVisible() && !mScrollTimer->isActive()) mScrollTimer->start(100);
    }
    else
    {
        mScrollMagnitude = 0;
        mScrollTimer->stop();
    }

    QList<WBThumbnailPixmap*> pixmapItems;
    foreach (QGraphicsItem *item, scene()->items(mapToScene(boundingFrame)))
    {
        WBThumbnailPixmap* sceneItem = dynamic_cast<WBThumbnailPixmap*>(item);
        if (sceneItem)
            pixmapItems.append(sceneItem);
    }

    int minDistance = 0;
    QGraphicsItem *underlyingItem = itemAt(event->pos());
    mClosestDropItem = dynamic_cast<WBThumbnailPixmap*>(underlyingItem);

    if (!mClosestDropItem)
    {
        foreach (WBThumbnailPixmap *item, pixmapItems)
        {
            qreal scale = item->transform().m11();
            QPointF itemCenter(
                        item->pos().x() + item->boundingRect().width() * scale / 2,
                        item->pos().y() + item->boundingRect().height() * scale / 2);

            int distance = (itemCenter.toPoint() - mapToScene(event->pos()).toPoint()).manhattanLength();
            if (!mClosestDropItem || distance < minDistance)
            {
                mClosestDropItem = item;
                minDistance = distance;
            }
        }
    }

    if (mClosestDropItem)
    {
        qreal scale = mClosestDropItem->transform().m11();

        QPointF itemCenter(
                    mClosestDropItem->pos().x() + mClosestDropItem->boundingRect().width() * scale / 2,
                    mClosestDropItem->pos().y() + mClosestDropItem->boundingRect().height() * scale / 2);

        mDropIsRight = mapToScene(event->pos()).x() > itemCenter.x();

        if (!mDropCaretRectItem && selectedItems().count() < mGraphicItems.count())
        {
            mDropCaretRectItem = new QGraphicsRectItem(0);
            scene()->addItem(mDropCaretRectItem);
            mDropCaretRectItem->setPen(QPen(Qt::darkGray));
            mDropCaretRectItem->setBrush(QBrush(Qt::lightGray));
        }

        QRectF dropCaretRect(
                    mDropIsRight ? mClosestDropItem->pos().x() + mClosestDropItem->boundingRect().width() * scale + spacing() / 2 - 1 : mClosestDropItem->pos().x() - spacing() / 2 - 1,
                    mClosestDropItem->pos().y(),
                    3,
                    mClosestDropItem->boundingRect().height() * scale);

        if (mDropCaretRectItem)
            mDropCaretRectItem->setRect(dropCaretRect);
    }

    event->acceptProposedAction();
}


void WBDocumentThumbnailWidget::dropEvent(QDropEvent *event)
{
    if (mScrollTimer->isActive())
    {
        mScrollMagnitude = 0;
        mScrollTimer->stop();
    }
    deleteDropCaret();

    if (mClosestDropItem)
    {
        int targetIndex = mDropIsRight ? mGraphicItems.indexOf(mClosestDropItem) + 1 : mGraphicItems.indexOf(mClosestDropItem);

        QList<WBMimeDataItem> mimeDataItems;
        if (event->mimeData()->hasFormat(WBApplication::mimeTypeUniboardPage))
        {
            const WBMimeData* mimeData = qobject_cast<const WBMimeData*>(event->mimeData());
            if (mimeData)
                mimeDataItems = mimeData->items();
        }

        if (1 == mimeDataItems.count() &&
                (mimeDataItems.at(0).sceneIndex() == mGraphicItems.indexOf(mClosestDropItem) ||
                 targetIndex == mimeDataItems.at(0).sceneIndex() ||
                 targetIndex == mimeDataItems.at(0).sceneIndex() + 1))
        {
            return;
        }

        int sourceIndexOffset = 0;
        int actualTargetIndex = targetIndex;
        for (int i = mimeDataItems.count() - 1; i >= 0; i--)
        {
            WBMimeDataItem sourceItem = mimeDataItems.at(i);
            int actualSourceIndex = sourceItem.sceneIndex();
            if (sourceItem.sceneIndex() >= targetIndex)
                actualSourceIndex += sourceIndexOffset;

            //event->acceptProposedAction();
            if (sourceItem.sceneIndex() < targetIndex)
            {
                if (actualSourceIndex != actualTargetIndex - 1)
                    emit sceneDropped(sourceItem.documentProxy(), actualSourceIndex, actualTargetIndex - 1);
                actualTargetIndex -= 1;
            }
            else
            {
                if (actualSourceIndex != actualTargetIndex)
                    emit sceneDropped(sourceItem.documentProxy(), actualSourceIndex, actualTargetIndex);
                sourceIndexOffset += 1;
            }
        }
    }
    WBThumbnailWidget::dropEvent(event);
}

void WBDocumentThumbnailWidget::deleteDropCaret()
{
    if (mDropCaretRectItem && scene())
    {
        scene()->removeItem(mDropCaretRectItem);
        delete mDropCaretRectItem;
        mDropCaretRectItem = 0;
    }
}

void WBDocumentThumbnailWidget::setGraphicsItems(const QList<QGraphicsItem*>& pGraphicsItems,
                                                 const QList<QUrl>& pItemPaths, const QStringList pLabels,
                                                 const QString& pMimeType)
{
    deleteDropCaret();

    WBThumbnailWidget::setGraphicsItems(pGraphicsItems, pItemPaths, pLabels, pMimeType);
}

void WBDocumentThumbnailWidget::setDragEnabled(bool enabled)
{
    mDragEnabled = enabled;
}

bool WBDocumentThumbnailWidget::dragEnabled() const
{
    return mDragEnabled;
}

void WBDocumentThumbnailWidget::hightlightItem(int index)
{
    if (0 <= index && index < mLabelsItems.length())
    {
        mLabelsItems.at(index)->highlight();
    }
    if (0 <= index && index < mGraphicItems.length())
    {
        WBSceneThumbnailPixmap *thumbnail = dynamic_cast<WBSceneThumbnailPixmap*>(mGraphicItems.at(index));
        if (thumbnail)
            thumbnail->highlight();
    }

    selectItemAt(index);
}
