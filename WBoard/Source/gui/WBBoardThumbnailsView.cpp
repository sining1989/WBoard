#include <QList>
#include <QPointF>
#include <QPixmap>
#include <QTransform>
#include <QScrollBar>
#include <QFontMetrics>
#include <QGraphicsItem>
#include <QGraphicsPixmapItem>

#include "core/WBApplication.h"
#include "WBBoardThumbnailsView.h"
#include "board/WBBoardController.h"
#include "adaptors/WBThumbnailAdaptor.h"
#include "adaptors/WBSvgSubsetAdaptor.h"
#include "document/WBDocumentController.h"
#include "domain/WBGraphicsScene.h"
#include "board/WBBoardPaletteManager.h"
#include "core/WBApplicationController.h"
#include "core/WBPersistenceManager.h"
#include "WBThumbnailView.h"

WBBoardThumbnailsView::WBBoardThumbnailsView(QWidget *parent, const char *name)
    : QGraphicsView(parent)
    , mThumbnailWidth(0)
    , mThumbnailMinWidth(60)
    , mMargin(20)
    , mDropSource(NULL)
    , mDropTarget(NULL)
    , mDropBar(new QGraphicsRectItem(0))
    , mLongPressInterval(350)
{
    setScene(new QGraphicsScene(this));    

    mDropBar->setPen(QPen(Qt::darkGray));
    mDropBar->setBrush(QBrush(Qt::lightGray));
    scene()->addItem(mDropBar);
    mDropBar->hide();

    setObjectName(name);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameShadow(QFrame::Plain);

    mThumbnailWidth = width() - 2*mMargin;

    mLongPressTimer.setInterval(mLongPressInterval);
    mLongPressTimer.setSingleShot(true);

    connect(WBApplication::boardController, SIGNAL(initThumbnailsRequired(WBDocumentContainer*)), this, SLOT(initThumbnails(WBDocumentContainer*)), Qt::UniqueConnection);
    connect(WBApplication::boardController, SIGNAL(addThumbnailRequired(WBDocumentContainer*, int)), this, SLOT(addThumbnail(WBDocumentContainer*, int)), Qt::UniqueConnection);
    connect(WBApplication::boardController, SIGNAL(moveThumbnailRequired(int, int)), this, SLOT(moveThumbnail(int, int)), Qt::UniqueConnection);
    connect(this, SIGNAL(moveThumbnailRequired(int, int)), this, SLOT(moveThumbnail(int, int)), Qt::UniqueConnection);
    connect(WBApplication::boardController, SIGNAL(updateThumbnailsRequired()), this, SLOT(updateThumbnails()), Qt::UniqueConnection);
    connect(WBApplication::boardController, SIGNAL(removeThumbnailRequired(int)), this, SLOT(removeThumbnail(int)), Qt::UniqueConnection);

    connect(&mLongPressTimer, SIGNAL(timeout()), this, SLOT(longPressTimeout()), Qt::UniqueConnection);

    connect(this, SIGNAL(mousePressAndHoldEventRequired(QPoint)), this, SLOT(mousePressAndHoldEvent(QPoint)), Qt::UniqueConnection);

    connect(WBApplication::boardController, SIGNAL(pageSelectionChanged(int)), this, SLOT(ensureVisibleThumbnail(int)), Qt::UniqueConnection);
    connect(WBApplication::boardController, SIGNAL(centerOnThumbnailRequired(int)), this, SLOT(centerOnThumbnail(int)), Qt::UniqueConnection);
}

void WBBoardThumbnailsView::moveThumbnail(int from, int to)
{
    mThumbnails.move(from, to);

    updateThumbnailsPos();
}

void WBBoardThumbnailsView::updateThumbnails()
{
    updateThumbnailsPos();
}

void WBBoardThumbnailsView::removeThumbnail(int i)
{
    WBDraggableThumbnailView* item = mThumbnails.at(i);

    scene()->removeItem(item->pageNumber());
    scene()->removeItem(item);

    mThumbnails.removeAt(i);

    updateThumbnailsPos();
}

WBDraggableThumbnailView* WBBoardThumbnailsView::createThumbnail(WBDocumentContainer* source, int i)
{
    WBApplication::showMessage(tr("Loading page (%1/%2)").arg(i+1).arg(source->selectedDocument()->pageCount()));

    WBGraphicsScene* pageScene = WBPersistenceManager::persistenceManager()->loadDocumentScene(source->selectedDocument(), i);
    WBThumbnailView* pageView = new WBThumbnailView(pageScene);

    return new WBDraggableThumbnailView(pageView, source->selectedDocument(), i);
}

void WBBoardThumbnailsView::addThumbnail(WBDocumentContainer* source, int i)
{
    WBDraggableThumbnailView* item = createThumbnail(source, i);
    mThumbnails.insert(i, item);

    scene()->addItem(item);
    scene()->addItem(item->pageNumber());

    updateThumbnailsPos();
}

void WBBoardThumbnailsView::clearThumbnails()
{
    for(int i = 0; i < mThumbnails.size(); i++)
    {
        scene()->removeItem(mThumbnails.at(i)->pageNumber());
        scene()->removeItem(mThumbnails.at(i));
        mThumbnails.at(i)->deleteLater();
    }

    mThumbnails.clear();
}

void WBBoardThumbnailsView::initThumbnails(WBDocumentContainer* source)
{
    clearThumbnails();

    for(int i = 0; i < source->selectedDocument()->pageCount(); i++)
    {
        mThumbnails.append(createThumbnail(source, i));

        scene()->addItem(mThumbnails.last());
        scene()->addItem(mThumbnails.last()->pageNumber());
    }

    updateThumbnailsPos();
}

void WBBoardThumbnailsView::centerOnThumbnail(int index)
{
    centerOn(mThumbnails.at(index));
}

void WBBoardThumbnailsView::ensureVisibleThumbnail(int index)
{
    ensureVisible(mThumbnails.at(index));
}

void WBBoardThumbnailsView::updateThumbnailsPos()
{    
    qreal thumbnailHeight = mThumbnailWidth / WBSettings::minScreenRatio;

    for (int i=0; i < mThumbnails.length(); i++)
    {
        mThumbnails.at(i)->setSceneIndex(i);
        mThumbnails.at(i)->setPageNumber(i);
        mThumbnails.at(i)->updatePos(mThumbnailWidth, thumbnailHeight);
    }

    scene()->setSceneRect(0, 0, scene()->itemsBoundingRect().size().width() - verticalScrollBar()->width(), scene()->itemsBoundingRect().size().height());

    update();
}

void WBBoardThumbnailsView::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);

    // Update the thumbnails width
    mThumbnailWidth = (width() > mThumbnailMinWidth) ? width() - verticalScrollBar()->width() - 2*mMargin : mThumbnailMinWidth;

    // Refresh the scene
    updateThumbnailsPos();

    emit WBApplication::boardController->centerOnThumbnailRequired(WBApplication::boardController->activeSceneIndex());
}

void WBBoardThumbnailsView::mousePressEvent(QMouseEvent *event)
{
    QGraphicsView::mousePressEvent(event);

    if (!event->isAccepted())
    {
        mLongPressTimer.start();
        mLastPressedMousePos = event->pos();

        WBDraggableThumbnailView* item = dynamic_cast<WBDraggableThumbnailView*>(itemAt(event->pos()));

        if (item)
        {
            WBApplication::boardController->persistViewPositionOnCurrentScene();
            WBApplication::boardController->persistCurrentScene();
            WBApplication::boardController->setActiveDocumentScene(item->sceneIndex());
            WBApplication::boardController->centerOn(WBApplication::boardController->activeScene()->lastCenter());
        }
    }
}

void WBBoardThumbnailsView::mouseMoveEvent(QMouseEvent *event)
{
    QGraphicsView::mouseMoveEvent(event);
}

void WBBoardThumbnailsView::longPressTimeout()
{
    if (QApplication::mouseButtons() != Qt::NoButton)
        emit mousePressAndHoldEventRequired(mLastPressedMousePos);

    mLongPressTimer.stop();
}

void WBBoardThumbnailsView::mousePressAndHoldEvent(QPoint pos)
{
    WBDraggableThumbnailView* item = dynamic_cast<WBDraggableThumbnailView*>(itemAt(pos));
    if (item)
    {
        mDropSource = item;
        mDropTarget = item;

        QPixmap pixmap = item->widget()->grab().scaledToWidth(mThumbnailWidth/2);

        QDrag *drag = new QDrag(this);
        drag->setMimeData(new QMimeData());
        drag->setPixmap(pixmap);
        drag->setHotSpot(QPoint(pixmap.width()/2, pixmap.height()/2));

        drag->exec();
    }   
}

void WBBoardThumbnailsView::mouseReleaseEvent(QMouseEvent *event)
{
    mLongPressTimer.stop();

    QGraphicsView::mouseReleaseEvent(event);
}

void WBBoardThumbnailsView::dragEnterEvent(QDragEnterEvent *event)
{
    mDropBar->show();

    if (event->source() == this)
    {
        event->setDropAction(Qt::MoveAction);
        event->accept();
    }
    else
    {
        event->acceptProposedAction();
    }
}

void WBBoardThumbnailsView::dragMoveEvent(QDragMoveEvent *event)
{        
    QPointF position = event->pos();

    //autoscroll during drag'n'drop
    QPointF scenePos = mapToScene(position.toPoint());
    int thumbnailHeight = mThumbnailWidth / WBSettings::minScreenRatio;
    QRectF thumbnailArea(0, scenePos.y() - thumbnailHeight/2, mThumbnailWidth, thumbnailHeight);

    ensureVisible(thumbnailArea);

    WBDraggableThumbnailView* item = dynamic_cast<WBDraggableThumbnailView*>(itemAt(position.toPoint()));
    if (item)
    {
        if (item != mDropTarget)
            mDropTarget = item;

        qreal scale = item->transform().m11();

        QPointF itemCenter(item->pos().x() + (item->boundingRect().width()-verticalScrollBar()->width()) * scale,
                           item->pos().y() + item->boundingRect().height() * scale / 2);

        bool dropAbove = mapToScene(position.toPoint()).y() < itemCenter.y();
        bool movingUp = mDropSource->sceneIndex() > item->sceneIndex();
        qreal y = 0;

        if (movingUp)
        {
            if (dropAbove)
            {
                y = item->pos().y() - WBSettings::thumbnailSpacing / 2;
                if (mDropBar->y() != y)
                    mDropBar->setRect(QRectF(item->pos().x(), y, mThumbnailWidth-verticalScrollBar()->width(), 3));
            }
        }
        else
        {
            if (!dropAbove)
            {
                y = item->pos().y() + item->boundingRect().height() * scale + WBSettings::thumbnailSpacing / 2;
                if (mDropBar->y() != y)
                    mDropBar->setRect(QRectF(item->pos().x(), y, mThumbnailWidth-verticalScrollBar()->width(), 3));
            }
        }
    }

    event->acceptProposedAction();
}

void WBBoardThumbnailsView::dropEvent(QDropEvent *event)
{
    Q_UNUSED(event);

    if (mDropSource->sceneIndex() != mDropTarget->sceneIndex())
        WBApplication::boardController->moveSceneToIndex(mDropSource->sceneIndex(), mDropTarget->sceneIndex());

    mDropSource = NULL;
    mDropTarget = NULL;

    mDropBar->setRect(QRectF());
    mDropBar->hide();
}
