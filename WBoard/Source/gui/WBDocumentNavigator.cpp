#include <QList>
#include <QPointF>
#include <QPixmap>
#include <QTransform>
#include <QScrollBar>
#include <QFontMetrics>
#include <QGraphicsItem>
#include <QGraphicsPixmapItem>

#include "core/WBApplication.h"
#include "WBDocumentNavigator.h"
#include "board/WBBoardController.h"
#include "adaptors/WBThumbnailAdaptor.h"
#include "adaptors/WBSvgSubsetAdaptor.h"
#include "document/WBDocumentController.h"
#include "domain/WBGraphicsScene.h"
#include "board/WBBoardPaletteManager.h"
#include "core/WBApplicationController.h"

#include "core/memcheck.h"

/**
 * \brief Constructor
 * @param parent as the parent widget
 * @param name as the object name
 */
WBDocumentNavigator::WBDocumentNavigator(QWidget *parent, const char *name):QGraphicsView(parent)
  , mScene(NULL)
  , mNbColumns(1)
  , mThumbnailWidth(0)
  , mThumbnailMinWidth(50)
  , mSelectedThumbnail(NULL)
  , mLastClickedThumbnail(NULL)
  , mDropSource(NULL)
  , mDropTarget(NULL)
  , mDropBar(new QGraphicsRectItem())
  , mLongPressInterval(350)
{
    setObjectName(name);
    mScene = new QGraphicsScene(this);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setScene(mScene);

    mThumbnailWidth = width() - 2*border();

    mDropBar->setPen(QPen(Qt::darkGray));
    mDropBar->setBrush(QBrush(Qt::lightGray));
    scene()->addItem(mDropBar);
    mDropBar->hide();

    mLongPressTimer.setInterval(mLongPressInterval);
    mLongPressTimer.setSingleShot(true);

    setFrameShadow(QFrame::Plain);

    connect(WBApplication::boardController, SIGNAL(documentThumbnailsUpdated(WBDocumentContainer*)), this, SLOT(generateThumbnails(WBDocumentContainer*)));
    connect(WBApplication::boardController, SIGNAL(documentPageUpdated(int)), this, SLOT(updateSpecificThumbnail(int)));
    connect(WBApplication::boardController, SIGNAL(pageSelectionChanged(int)), this, SLOT(onScrollToSelectedPage(int)));

    connect(&mLongPressTimer, SIGNAL(timeout()), this, SLOT(longPressTimeout()), Qt::UniqueConnection);

    connect(this, SIGNAL(mousePressAndHoldEventRequired()), this, SLOT(mousePressAndHoldEvent()), Qt::UniqueConnection);
}

/**
 * \brief Destructor
 */
WBDocumentNavigator::~WBDocumentNavigator()
{
    if(NULL != mScene)
    {
        delete mScene;
        mScene = NULL;
    }
}

/**
 * \brief Generate the thumbnails
 */
void WBDocumentNavigator::generateThumbnails(WBDocumentContainer* source)
{
    mThumbsWithLabels.clear();
    int selectedIndex = -1;
    int lastClickedIndex = -1;
    if (mLastClickedThumbnail)
    {
        lastClickedIndex = mLastClickedThumbnail->sceneIndex();
    }

    QList<QGraphicsItem*> graphicsItemList = mScene->items();
    for(int i = 0; i < graphicsItemList.size(); i+=1)
    {
        QGraphicsItem* item = graphicsItemList.at(i);
        if(item->isSelected())
            selectedIndex = i;

        if (item != mDropBar)
        {
            mScene->removeItem(item);
            delete item;
            item = NULL;
        }
    }

    for(int i = 0; i < source->selectedDocument()->pageCount(); i++)
    {
        //claudio This is a very bad hack and shows a architectural problem
        // source->selectedDocument()->pageCount()  !=   source->pageCount()
        if(i>=source->pageCount() || !source->pageAt(i))
            source->insertThumbPage(i);

        const QPixmap* pix = source->pageAt(i);
        Q_ASSERT(!pix->isNull());
        int pageIndex = WBDocumentContainer::pageFromSceneIndex(i);

        WBSceneThumbnailNavigPixmap* pixmapItem = new WBSceneThumbnailNavigPixmap(*pix, source->selectedDocument(), i);

        QString label = tr("Page %0").arg(pageIndex);
        WBThumbnailTextItem *labelItem = new WBThumbnailTextItem(label);

        WBImgTextThumbnailElement thumbWithText(pixmapItem, labelItem);
        thumbWithText.setBorder(border());
        mThumbsWithLabels.append(thumbWithText);

        if (lastClickedIndex == i)
            mLastClickedThumbnail = pixmapItem;

        mScene->addItem(pixmapItem);
        mScene->addItem(labelItem);
    }

    if (selectedIndex >= 0 && selectedIndex < mThumbsWithLabels.count())
        mSelectedThumbnail = mThumbsWithLabels.at(selectedIndex).getThumbnail();
    else
        mSelectedThumbnail = NULL;

    // Draw the items
    refreshScene();
}

void WBDocumentNavigator::onScrollToSelectedPage(int index)
{
    int c  = 0;
    foreach(WBImgTextThumbnailElement el, mThumbsWithLabels)
    {
        if (c==index)
        {
            el.getThumbnail()->setSelected(true);
            mSelectedThumbnail = el.getThumbnail();
        }
        else
        {
            el.getThumbnail()->setSelected(false);
        }
        c++;
    }
    if(NULL != mSelectedThumbnail)
        ensureVisible(mSelectedThumbnail);
}

/**
 * \brief Refresh the given thumbnail
 * @param iPage as the given page related thumbnail
 */
void WBDocumentNavigator::updateSpecificThumbnail(int iPage)
{
    const QPixmap* pix = WBApplication::boardController->pageAt(iPage);
    WBSceneThumbnailNavigPixmap* newItem = new WBSceneThumbnailNavigPixmap(*pix, WBApplication::boardController->selectedDocument(), iPage);

    // Get the old thumbnail
    WBSceneThumbnailNavigPixmap* oldItem = mThumbsWithLabels.at(iPage).getThumbnail();
    if(NULL != oldItem)
    {
        mScene->removeItem(oldItem);
        mScene->addItem(newItem);
        mThumbsWithLabels[iPage].setThumbnail(newItem);
        if (mLastClickedThumbnail == oldItem)
            mLastClickedThumbnail = newItem;
        delete oldItem;
        oldItem = NULL;
    }

}

/**
 * \brief Put the element in the right place in the scene.
 */
void WBDocumentNavigator::refreshScene()
{
    qreal thumbnailHeight = mThumbnailWidth / WBSettings::minScreenRatio;

    for(int i = 0; i < mThumbsWithLabels.size(); i++)
    {
        // Get the item
        WBImgTextThumbnailElement& item = mThumbsWithLabels[i];
        int columnIndex = i % mNbColumns;
        int rowIndex = i / mNbColumns;
        item.Place(rowIndex, columnIndex, mThumbnailWidth, thumbnailHeight);
    }
    scene()->setSceneRect(scene()->itemsBoundingRect());
}

/**
 * \brief  Set the number of thumbnails columns
 * @param nbColumns as the number of columns
 */
void WBDocumentNavigator::setNbColumns(int nbColumns)
{
    mNbColumns = nbColumns;
}

/**
 * \brief Get the number of columns
 * @return the number of thumbnails columns
 */
int WBDocumentNavigator::nbColumns()
{
    return mNbColumns;
}

/**
 * \brief Set the thumbnails minimum width
 * @param width as the minimum width
 */
void WBDocumentNavigator::setThumbnailMinWidth(int width)
{
    mThumbnailMinWidth = width;
}

/**
 * \brief Get the thumbnails minimum width
 * @return the minimum thumbnails width
 */
int WBDocumentNavigator::thumbnailMinWidth()
{
    return mThumbnailMinWidth;
}

/**
 * \brief Get the border size
 * @return the border size in pixels
 */
int WBDocumentNavigator::border()
{
    return 10;
}

/**
 * \brief Handle the resize event
 * @param event as the resize event
 */
void WBDocumentNavigator::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);

    // Update the thumbnails width
	mThumbnailWidth = (width() > mThumbnailMinWidth) ? width() - 2*border() : mThumbnailMinWidth;

    if(mSelectedThumbnail)
        ensureVisible(mSelectedThumbnail);

    // Refresh the scene
    refreshScene();
}

/**
 * \brief Handle the mouse press event
 * @param event as the mouse event
 */
void WBDocumentNavigator::mousePressEvent(QMouseEvent *event)
{
    QGraphicsView::mousePressEvent(event);

    if (!event->isAccepted())
    {
        mLongPressTimer.start();
        mLastPressedMousePos = event->pos();

        mLastClickedThumbnail = clickedThumbnail(mLastPressedMousePos);

        if(mLastClickedThumbnail)
        {
            WBApplication::boardController->persistViewPositionOnCurrentScene();
            WBApplication::boardController->persistCurrentScene();
            WBApplication::boardController->setActiveDocumentScene(mLastClickedThumbnail->sceneIndex());
            WBApplication::boardController->centerOn(WBApplication::boardController->activeScene()->lastCenter());
        }
    }
}

WBSceneThumbnailNavigPixmap* WBDocumentNavigator::clickedThumbnail(const QPoint pos) const
{
    WBSceneThumbnailNavigPixmap* clickedThumbnail = NULL;

    QGraphicsItem* clickedItem = itemAt(pos);

    if(clickedItem)
    {
        clickedThumbnail = dynamic_cast<WBSceneThumbnailNavigPixmap*>(clickedItem);

        if(!clickedThumbnail)
        {
            // If we fall here we may have clicked on the label instead of the thumbnail
            WBThumbnailTextItem* clickedTextItem = dynamic_cast<WBThumbnailTextItem*>(clickedItem);
            if(clickedTextItem)
            {
                for(int i = 0; i < mThumbsWithLabels.size(); i++)
                {
                    const WBImgTextThumbnailElement& el = mThumbsWithLabels.at(i);
                    if(el.getCaption() == clickedTextItem)
                    {
                        clickedThumbnail = el.getThumbnail();
                        break;
                    }
                }
            }
        }
    }

    return clickedThumbnail;
}

void WBDocumentNavigator::mouseReleaseEvent(QMouseEvent *event)
{
    event->accept();
    mLongPressTimer.stop();
}

void WBDocumentNavigator::longPressTimeout()
{
    if (QApplication::mouseButtons() != Qt::NoButton)
        emit mousePressAndHoldEventRequired();

    mLongPressTimer.stop();
}

void WBDocumentNavigator::mousePressAndHoldEvent()
{
    if (mLastClickedThumbnail)
    {
        mDropSource = mLastClickedThumbnail;
        mDropTarget = mLastClickedThumbnail;

        QPixmap pixmap = mLastClickedThumbnail->pixmap().scaledToWidth(mThumbnailWidth/2);

        QDrag *drag = new QDrag(this);
        drag->setMimeData(new QMimeData());
        drag->setPixmap(pixmap);
        drag->setHotSpot(QPoint(pixmap.width()/2, pixmap.height()/2));

        drag->exec();
    }
}

void WBDocumentNavigator::dragEnterEvent(QDragEnterEvent *event)
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

void WBDocumentNavigator::dragMoveEvent(QDragMoveEvent *event)
{
    QPointF position = event->pos();

    //autoscroll during drag'n'drop
    QPointF scenePos = mapToScene(position.toPoint());
    int thumbnailHeight = mThumbnailWidth / WBSettings::minScreenRatio;
    QRectF thumbnailArea(0, scenePos.y() - thumbnailHeight/2, mThumbnailWidth, thumbnailHeight);

    ensureVisible(thumbnailArea);

    WBSceneThumbnailNavigPixmap* item = dynamic_cast<WBSceneThumbnailNavigPixmap*>(itemAt(position.toPoint()));
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

void WBDocumentNavigator::dropEvent(QDropEvent *event)
{
    Q_UNUSED(event);

    if (mDropSource->sceneIndex() != mDropTarget->sceneIndex())
        WBApplication::boardController->moveSceneToIndex(mDropSource->sceneIndex(), mDropTarget->sceneIndex());

    mDropSource = NULL;
    mDropTarget = NULL;
    mLastClickedThumbnail = NULL;

    mDropBar->setRect(QRectF());
    mDropBar->hide();
}
