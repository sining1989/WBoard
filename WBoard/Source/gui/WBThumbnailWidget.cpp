#include <QString>
#include <QCursor>
#include <QGraphicsRectItem>

#include "WBThumbnailWidget.h"
#include "WBRubberBand.h"
#include "WBMainWindow.h"

#include <QWidget>

#include "board/WBBoardController.h"

#include "core/WBSettings.h"
#include "core/WBApplication.h"

#include "document/WBDocumentProxy.h"
#include "document/WBDocumentController.h"

#include "board/WBBoardPaletteManager.h"

#include "core/memcheck.h"

WBThumbnailWidget::WBThumbnailWidget(QWidget* parent)
    : QGraphicsView(parent)
    , mThumbnailWidth(WBSettings::defaultThumbnailWidth)
    , mSpacing(WBSettings::thumbnailSpacing)
    , mLastSelectedThumbnail(0)
    , mSelectionSpan(0)
    , mPrevLassoRect(QRect())
    , mLassoRectItem(0)

{
    // By default, the drag is possible
    bCanDrag = true;
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);
    setFrameShape(QFrame::NoFrame);
    setScene(&mThumbnailsScene);

    setAlignment(Qt::AlignLeft | Qt::AlignTop);

    connect(&mThumbnailsScene, SIGNAL(selectionChanged()), this, SLOT(sceneSelectionChanged()));
}


WBThumbnailWidget::~WBThumbnailWidget()
{
    disconnect(&mThumbnailsScene, SIGNAL(selectionChanged()));
}


void WBThumbnailWidget::setThumbnailWidth(qreal pThumbnailWidth)
{
    mThumbnailWidth = pThumbnailWidth;

    refreshScene();
}


void WBThumbnailWidget::setSpacing(qreal pSpacing)
{
    mSpacing = pSpacing;

    refreshScene();
}


void WBThumbnailWidget::setGraphicsItems(const QList<QGraphicsItem*>& pGraphicsItems
        , const QList<QUrl>& pItemsPaths
        , const QStringList pLabels
        , const QString& pMimeType)
{
    Q_ASSERT(pItemsPaths.count() == pLabels.count());
    mGraphicItems = pGraphicsItems;
    mItemsPaths = pItemsPaths;
    mMimeType = pMimeType;
    mLabels = pLabels;

    foreach(QGraphicsItem* it, mThumbnailsScene.items())
    {
        mThumbnailsScene.removeItem(it, true);
    }

    // set lasso to 0 as it has been cleared as well
    mLassoRectItem = 0;

    foreach (QGraphicsItem* item, pGraphicsItems)
    {
        if (item->scene() != &mThumbnailsScene){
            mThumbnailsScene.addItem(item);
        }
    }

    mLabelsItems.clear();

    foreach (const QString label, pLabels)
    {
        QFontMetrics fm(font());
        WBThumbnailTextItem *labelItem =
            new WBThumbnailTextItem(label); // deleted while replace or by the scene destruction

        mThumbnailsScene.addItem(labelItem);
        mLabelsItems << labelItem;
    }

    refreshScene();

    mLastSelectedThumbnail = 0;
}


void WBThumbnailWidget::refreshScene()
{
    int nbColumns = (geometry().width() - mSpacing) / (mThumbnailWidth + mSpacing);

    int labelSpacing = 0;

    if (mLabelsItems.size() > 0)
    {
        QFontMetrics fm(mLabelsItems.at(0)->font());
        labelSpacing = WBSettings::thumbnailSpacing + fm.height();
    }
    nbColumns = qMax(nbColumns, 1);

    qreal thumbnailHeight = mThumbnailWidth / WBSettings::minScreenRatio;

    for (int i = 0; i < mGraphicItems.size(); i++)
    {
        QGraphicsItem* item = mGraphicItems.at(i);

        qreal scaleWidth = mThumbnailWidth / item->boundingRect().width();
        qreal scaleHeight = thumbnailHeight / item->boundingRect().height();

        qreal scaleFactor = qMin(scaleWidth, scaleHeight);

        //bitmap should not be stretched
        WBThumbnail* pix = dynamic_cast<WBThumbnail*>(item);
        if (pix)
            scaleFactor = qMin(scaleFactor, 1.0);

        QTransform transform;
        transform.scale(scaleFactor, scaleFactor);

        item->setTransform(transform);

        item->setFlag(QGraphicsItem::ItemIsSelectable, true);

        int columnIndex = i % nbColumns;
        int rowIndex = i / nbColumns;

        if (pix)
        {
            pix->setColumn(columnIndex);
            pix->setRow(rowIndex);
        }

        int w = item->boundingRect().width();
        int h = item->boundingRect().height();
        QPointF pos(
                mSpacing + (mThumbnailWidth - w * scaleFactor) / 2 + columnIndex * (mThumbnailWidth + mSpacing),
                mSpacing + rowIndex * (thumbnailHeight + mSpacing + labelSpacing) + (thumbnailHeight - h * scaleFactor) / 2);

        item->setPos(pos);

        if (mLabelsItems.size() > i)
        {
            QFontMetrics fm(mLabelsItems.at(i)->font(), this);
            QString elidedText = fm.elidedText(mLabels.at(i), Qt::ElideRight, mThumbnailWidth);

            mLabelsItems.at(i)->setPlainText(elidedText);
            mLabelsItems.at(i)->setWidth(fm.width(elidedText) + 2 * mLabelsItems.at(i)->document()->documentMargin());

            pos.setY(pos.y() + (thumbnailHeight + h * scaleFactor) / 2 + 5);
            qreal labelWidth = fm.width(elidedText);
            pos.setX(mSpacing + (mThumbnailWidth - labelWidth) / 2 + columnIndex * (mThumbnailWidth + mSpacing));
            mLabelsItems.at(i)->setPos(pos);
        }
    }

    QScrollBar *vertScrollBar = verticalScrollBar();
    int scrollBarThickness = 0;
    if (vertScrollBar && vertScrollBar->isVisible())
        scrollBarThickness = vertScrollBar->width();

    setSceneRect(0, 0,
            geometry().width() - scrollBarThickness,
            mSpacing + ((((mGraphicItems.size() - 1) / nbColumns) + 1) * (thumbnailHeight + mSpacing + labelSpacing)));
}


QList<QGraphicsItem*> WBThumbnailWidget::selectedItems()
{
    QList<QGraphicsItem*> sortedSelectedItems = mThumbnailsScene.selectedItems();
    qSort(sortedSelectedItems.begin(), sortedSelectedItems.end(), thumbnailLessThan);
    return sortedSelectedItems;
}


void WBThumbnailWidget::mousePressEvent(QMouseEvent *event)
{
    mClickTime = QTime::currentTime();
    mMousePressPos = event->pos();

    WBThumbnailPixmap* sceneItem = dynamic_cast<WBThumbnailPixmap*>(itemAt(mMousePressPos));
    if(sceneItem==NULL)
    {
        event->ignore();
        return;
    }

    mMousePressScenePos = mapToScene(mMousePressPos);
    QGraphicsItem* underlyingItem = itemAt(mMousePressPos);
    WBThumbnail *previousSelectedThumbnail = mLastSelectedThumbnail;

    if (!dynamic_cast<WBThumbnail*>(underlyingItem))
    {
        deleteLasso();

        WBRubberBand rubberBand(QRubberBand::Rectangle);
        QStyleOption option;
        option.initFrom(&rubberBand);

        mPrevLassoRect = QRect();
        mLassoRectItem = new QGraphicsRectItem(0);
        scene()->addItem(mLassoRectItem);

#ifdef Q_OS_OSX
        // The following code must stay in synch with <Qt installation folder>\src\gui\styles\qmacstyle_mac.mm
        QColor strokeColor;
        strokeColor.setHsvF(0, 0, 0.86, 1.0);
        mLassoRectItem->setPen(QPen(strokeColor));
        QColor fillColor(option.palette.color(QPalette::Disabled, QPalette::Highlight));
        fillColor.setHsvF(0, 0, 0.53, 0.25);
        mLassoRectItem->setBrush(fillColor);
#else
        // The following code must stay in synch with <Qt installation folder>\src\gui\styles\qwindowsxpstyle.cpp
        QColor highlight = option.palette.color(QPalette::Active, QPalette::Highlight);
        mLassoRectItem->setPen(highlight.darker(120));
        QColor dimHighlight(qMin(highlight.red() / 2 + 110, 255),
                            qMin(highlight.green() / 2 + 110, 255),
                            qMin(highlight.blue() / 2 + 110, 255),
                            127);
        mLassoRectItem->setBrush(dimHighlight);
#endif

        mLassoRectItem->setZValue(10000);
        mLassoRectItem->setRect(QRectF(mMousePressScenePos, QSizeF()));

        if (Qt::ControlModifier & event->modifiers() || Qt::ShiftModifier & event->modifiers())
        {
           // mSelectedThumbnailItems = selectedItems().toSet();
            return;
        }

        mSelectedThumbnailItems.clear();
        mPreviouslyIncrementalSelectedItemsX.clear();
        mPreviouslyIncrementalSelectedItemsY.clear();
        QGraphicsView::mousePressEvent(event);
    }
    else if (Qt::ShiftModifier & event->modifiers())
    {
        if (previousSelectedThumbnail)
        {
            QGraphicsItem* previousSelectedItem = dynamic_cast<QGraphicsItem*>(previousSelectedThumbnail);
            if (previousSelectedItem)
            {
                int index1 = mGraphicItems.indexOf(previousSelectedItem);
                int index2 = mGraphicItems.indexOf(underlyingItem);
                if (-1 == index2)
                {
                    mSelectedThumbnailItems = selectedItems().toSet();
                    return;
                }
                mSelectionSpan = index2 - index1;
                selectItems(qMin(index1, index2), mSelectionSpan < 0 ? - mSelectionSpan + 1 : mSelectionSpan + 1);
                return;
            }
        }
    }
    else
    {
        mLastSelectedThumbnail = dynamic_cast<WBThumbnail*>(underlyingItem);
        if (!underlyingItem->isSelected())
        {
            int index = mGraphicItems.indexOf(underlyingItem);
            selectItemAt(index, Qt::ControlModifier & event->modifiers());
        }
        else
        {
            QGraphicsView::mousePressEvent(event);
        }
        if (!mLastSelectedThumbnail && mGraphicItems.count() > 0)
            mLastSelectedThumbnail = dynamic_cast<WBThumbnail*>(mGraphicItems.at(0));
        mSelectionSpan = 0;
        return;
    }
}


void WBThumbnailWidget::mouseMoveEvent(QMouseEvent *event)
{
    int distance = (mMousePressPos - event->pos()).manhattanLength();

    if (0 == (event->buttons() & Qt::LeftButton) || distance < QApplication::startDragDistance())
        return;

    if (mLassoRectItem)
    {
        bSelectionInProgress = true;
        int incrementLassoMinWidth = 2;
        QPointF currentScenePos = mapToScene(event->pos());
        QRectF lassoRect(
            qMin(mMousePressScenePos.x(), currentScenePos.x()), qMin(mMousePressScenePos.y(), currentScenePos.y()),
            qAbs(mMousePressScenePos.x() - currentScenePos.x()), qAbs(mMousePressScenePos.y() - currentScenePos.y()));
        if (QPoint() == prevMoveMousePos)
            prevMoveMousePos = currentScenePos;
        QRectF incrementXSelection(
            qMin(prevMoveMousePos.x(), currentScenePos.x()), qMin(mMousePressScenePos.y(), currentScenePos.y()),
            qAbs(prevMoveMousePos.x() - currentScenePos.x())+incrementLassoMinWidth, qAbs(mMousePressScenePos.y() - currentScenePos.y()));
        QRectF incrementYSelection(
            qMin(mMousePressScenePos.x(), currentScenePos.x()), qMin(prevMoveMousePos.y(), currentScenePos.y()),
            qAbs(mMousePressScenePos.x() - currentScenePos.x()), qAbs(prevMoveMousePos.y() - currentScenePos.y())+incrementLassoMinWidth);

        prevMoveMousePos = currentScenePos;
        mLassoRectItem->setRect(lassoRect);

        QSet<QGraphicsItem*> lassoSelectedThumbnailItems;

        QSet<QGraphicsItem*> toUnset;
        QSet<QGraphicsItem*> toSet;

        // for horizontal moving
        QSet<QGraphicsItem*> incSelectedItemsX = scene()->items(incrementXSelection, Qt::IntersectsItemBoundingRect).toSet();
        foreach (QGraphicsItem *lassoSelectedItem, incSelectedItemsX)
        {
            if (lassoSelectedItem)
            {
                WBThumbnailPixmap *thumbnailItem = dynamic_cast<WBThumbnailPixmap*>(lassoSelectedItem);
                if (thumbnailItem)
                     lassoSelectedThumbnailItems += lassoSelectedItem;
            }
        }

        if(lassoRect.width() < mPrevLassoRect.width())
        {
            if (!lassoSelectedThumbnailItems.contains(mPreviouslyIncrementalSelectedItemsX))
                toUnset += mPreviouslyIncrementalSelectedItemsX - lassoSelectedThumbnailItems;

        }
        mPreviouslyIncrementalSelectedItemsX = lassoSelectedThumbnailItems;

        toSet += lassoSelectedThumbnailItems + mPreviouslyIncrementalSelectedItemsX;


        lassoSelectedThumbnailItems.clear();

        // for vertical moving

        QSet<QGraphicsItem*> incSelectedItemsY = scene()->items(incrementYSelection, Qt::IntersectsItemBoundingRect).toSet();
        foreach (QGraphicsItem *lassoSelectedItem, incSelectedItemsY)
        {
            if (lassoSelectedItem)
            {
                WBThumbnailPixmap *thumbnailItem = dynamic_cast<WBThumbnailPixmap*>(lassoSelectedItem);

                if (thumbnailItem)
                    lassoSelectedThumbnailItems += lassoSelectedItem;
            }
        }

        if(lassoRect.height() < mPrevLassoRect.height())
        {
            if (!lassoSelectedThumbnailItems.contains(mPreviouslyIncrementalSelectedItemsY))
                toUnset += mPreviouslyIncrementalSelectedItemsY - lassoSelectedThumbnailItems;

        }
        mPreviouslyIncrementalSelectedItemsY = lassoSelectedThumbnailItems;


        toSet += lassoSelectedThumbnailItems + mPreviouslyIncrementalSelectedItemsY;


        toSet -= toUnset;

        foreach (QGraphicsItem *item, toSet)
        {
            item->setSelected(true);
        }

        foreach (QGraphicsItem *item, toUnset)
        {
            item->setSelected(false);
        }

        mSelectedThumbnailItems += lassoSelectedThumbnailItems;
        mPrevLassoRect = lassoRect;

        if (Qt::ControlModifier & event->modifiers())
        {
            for (int i = 0; i < mSelectedThumbnailItems.count()-1; i++)
            {
                mSelectedThumbnailItems.values().at(i)->setSelected(true);
            }
        }
    }
    else
    {
        bSelectionInProgress = false;
        if (0 == selectedItems().size())
            return;

        if(bCanDrag)
        {
            QDrag *drag = new QDrag(this);
            QMimeData *mime = new QMimeData();

            if (mMimeType.length() > 0)
                mime->setData(mMimeType, QByteArray()); // trick the d&d system to register our own mime type

            drag->setMimeData(mime);

            QList<QUrl> qlElements;

            foreach (QGraphicsItem* item, selectedItems())
            {
                if (mGraphicItems.contains(item))
                {
                    if (mGraphicItems.indexOf(item) <= mItemsPaths.size()){
                        qlElements << mItemsPaths.at(mGraphicItems.indexOf(item));
                    }
                }
            }

            if (qlElements.size() > 0){
                            mime->setUrls(qlElements);
                            drag->setMimeData(mime);
                            drag->exec(Qt::CopyAction);
            }
        }
    }

    QGraphicsView::mouseMoveEvent(event);
}


void WBThumbnailWidget::mouseReleaseEvent(QMouseEvent *event)
{
    int elapsedTimeSincePress = mClickTime.elapsed();
    prevMoveMousePos = QPoint();
    deleteLasso();
    QGraphicsView::mouseReleaseEvent(event);

    if(elapsedTimeSincePress < STARTDRAGTIME) {
        emit mouseClick(itemAt(event->pos()), 0);
    }
}


void WBThumbnailWidget::keyPressEvent(QKeyEvent *event)
{
    if (mLastSelectedThumbnail)
    {
        QGraphicsItem *lastSelectedGraphicsItem = dynamic_cast<QGraphicsItem*>(mLastSelectedThumbnail);
        if (!lastSelectedGraphicsItem) return;
        int startSelectionIndex = mGraphicItems.indexOf(lastSelectedGraphicsItem);
        int previousSelectedThumbnailIndex = startSelectionIndex + mSelectionSpan;

        switch (event->key())
        {
        case Qt::Key_Down:
        case Qt::Key_Up:
            {
                if (rowCount() <= 1) break;
                if (Qt::ShiftModifier & event->modifiers())
                {
                    int endSelectionIndex;
                    if (Qt::Key_Down == event->key())
                    {
                        endSelectionIndex = previousSelectedThumbnailIndex + columnCount();
                        if (endSelectionIndex >= mGraphicItems.count()) break;
                    }
                    else
                    {
                        endSelectionIndex = previousSelectedThumbnailIndex - columnCount();
                        if (endSelectionIndex < 0) break;
                    }

                    int startIndex = startSelectionIndex < endSelectionIndex ? startSelectionIndex : endSelectionIndex;
                    int count = startSelectionIndex < endSelectionIndex ? endSelectionIndex - startSelectionIndex + 1 : startSelectionIndex - endSelectionIndex + 1;
                    mSelectionSpan = startSelectionIndex < endSelectionIndex ? (count - 1) : - (count - 1);
                    selectItems(startIndex, count);
                }
                else
                {
                    int toSelectIndex;
                    if (Qt::Key_Down == event->key())
                    {
                        toSelectIndex = previousSelectedThumbnailIndex + columnCount();
                        if (toSelectIndex >= mGraphicItems.count()) break;
                    }
                    else
                    {
                        toSelectIndex = previousSelectedThumbnailIndex - columnCount();
                        if (toSelectIndex < 0) break;
                    }

                    selectItemAt(toSelectIndex, Qt::ControlModifier & event->modifiers());
                    mSelectionSpan = 0;
                }
            }
            break;

        case Qt::Key_Left:
        case Qt::Key_Right:
            {
                QGraphicsItem *previousSelectedItem = mGraphicItems.at(previousSelectedThumbnailIndex);
                WBThumbnail *previousSelectedThumbnail = dynamic_cast<WBThumbnail*>(previousSelectedItem);
                if (!previousSelectedThumbnail) break;

                if (Qt::Key_Left == event->key())
                {
                    if (0 == previousSelectedThumbnail->column()) break;
                }
                else
                {
                    if (previousSelectedThumbnail->column() == columnCount() - 1 ||
                        previousSelectedThumbnailIndex == mGraphicItems.count() - 1) break;
                }

                if (Qt::ShiftModifier & event->modifiers())
                {
                    int endSelectionIndex;
                    if (Qt::Key_Left == event->key())
                    {
                        endSelectionIndex = previousSelectedThumbnailIndex - 1;
                        if (endSelectionIndex < 0) break;
                    }
                    else
                    {
                        endSelectionIndex = previousSelectedThumbnailIndex + 1;
                        if (endSelectionIndex >= mGraphicItems.count()) break;
                    }

                    int startIndex = startSelectionIndex < endSelectionIndex ? startSelectionIndex : endSelectionIndex;
                    int count = startSelectionIndex < endSelectionIndex ? endSelectionIndex - startSelectionIndex + 1 : startSelectionIndex - endSelectionIndex + 1;
                    mSelectionSpan = startSelectionIndex < endSelectionIndex ? (count - 1) : - (count - 1);
                    selectItems(startIndex, count);
                }
                else
                {
                    if (Qt::Key_Left == event->key())
                        selectItemAt(previousSelectedThumbnailIndex - 1, Qt::ControlModifier & event->modifiers());
                    else
                        selectItemAt(previousSelectedThumbnailIndex + 1, Qt::ControlModifier & event->modifiers());

                    mSelectionSpan = 0;
                }
            }
            break;

        case Qt::Key_Home:
            {
                if (Qt::ShiftModifier & event->modifiers())
                {
                    mSelectionSpan = - startSelectionIndex;
                    selectItems(0, startSelectionIndex + 1);
                }
                else
                {
                    selectItemAt(0, Qt::ControlModifier & event->modifiers());
                    mSelectionSpan = 0;
                }
            }
            break;

        case Qt::Key_End:
            {
                if (Qt::ShiftModifier & event->modifiers())
                {
                    mSelectionSpan = mGraphicItems.count() - startSelectionIndex - 1;
                    selectItems(startSelectionIndex, mSelectionSpan + 1);
                }
                else
                {
                    selectItemAt(mGraphicItems.count() - 1, Qt::ControlModifier & event->modifiers());
                    mSelectionSpan = 0;
                }
            }
            break;
        case Qt::Key_A:
            {
                if (Qt::ControlModifier & event->modifiers())
                    selectAll();
            }
            break;
        }
    }
    QGraphicsView::keyPressEvent(event);
}


void WBThumbnailWidget::focusInEvent(QFocusEvent *event)
{
    Q_UNUSED(event);

    if (0 == selectedItems().count() && mGraphicItems.count() > 0 && Qt::TabFocusReason == event->reason())
    {
        selectItemAt(0);
        mSelectionSpan = 0;
    }
}


void WBThumbnailWidget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);

    refreshScene();

    emit resized();
}


void WBThumbnailWidget::sceneSelectionChanged()
{
    emit selectionChanged();
}


void WBThumbnailWidget::selectItemAt(int pIndex, bool extend)
{
    QGraphicsItem* itemToSelect = 0;

    if (pIndex >= 0 && pIndex < mGraphicItems.size())
        itemToSelect = mGraphicItems.at(pIndex);

    foreach (QGraphicsItem* item, items())
    {
        if (item == itemToSelect)
        {
            mLastSelectedThumbnail = dynamic_cast<WBThumbnail*>(item);
            item->setSelected(true);
            ensureVisible(item);
        }
        else if (!extend)
        {
            item->setSelected(false);
        }
    }
}

void WBThumbnailWidget::unselectItemAt(int pIndex)
{
    if (pIndex >= 0 && pIndex < mGraphicItems.size())
    {
        QGraphicsItem *itemToUnselect = mGraphicItems.at(pIndex);
        itemToUnselect->setSelected(false);
    }
}


void WBThumbnailWidget::selectItems(int startIndex, int count)
{
    for (int i = 0; i < mGraphicItems.count(); i++)
    {
        mGraphicItems.at(i)->setSelected(i >= startIndex && i < startIndex + count);
    }
}


void WBThumbnailWidget::selectAll()
{
    foreach (QGraphicsItem* item, mGraphicItems)
    {
        item->setSelected(true);
    }
}

int WBThumbnailWidget::rowCount() const
{
    WBThumbnail *lastThumbnail = dynamic_cast<WBThumbnail*>(mGraphicItems.last());
    return lastThumbnail ? lastThumbnail->row() + 1 : 0;
}

int WBThumbnailWidget::columnCount() const
{
    WBThumbnail *lastThumbnail = dynamic_cast<WBThumbnail*>(mGraphicItems.last());
    if (!lastThumbnail) return 0;
    int lastRow = lastThumbnail->row();
    int lastColumn = lastThumbnail->column();
    return lastRow > 0 ? (mGraphicItems.count() - lastColumn - 1) / lastRow : mGraphicItems.count();
}


void WBThumbnailWidget::mouseDoubleClickEvent(QMouseEvent * event)
{
    QGraphicsItem* item = itemAt(event->pos());

    if (item)
    {
        int index = mGraphicItems.indexOf(item);
        emit mouseDoubleClick(item, index);
    }
}


bool WBThumbnailWidget::thumbnailLessThan(QGraphicsItem* item1, QGraphicsItem* item2)
{
    WBThumbnail *thumbnail1 = dynamic_cast<WBThumbnail*>(item1);
    WBThumbnail *thumbnail2 = dynamic_cast<WBThumbnail*>(item2);
    if (thumbnail1 && thumbnail2)
    {
        if (thumbnail1->row() != thumbnail2->row())
            return thumbnail1->row() < thumbnail2->row();
        else
            return thumbnail1->column() < thumbnail2->column();
    }
    return false;
}

void WBThumbnailWidget::deleteLasso()
{
    if (mLassoRectItem && scene())
    {
        scene()->removeItem(mLassoRectItem);
        delete mLassoRectItem;
        mLassoRectItem = 0;
    }
}


WBThumbnail::WBThumbnail()
    : mAddedToScene(false)
{
    mSelectionItem = new QGraphicsRectItem(0, 0, 0, 0);
    mSelectionItem->setPen(QPen(WBSettings::treeViewBackgroundColor, 8));

}

WBThumbnail::~WBThumbnail()
{
    if (mSelectionItem && !mAddedToScene)
        delete mSelectionItem;
}

WBSceneThumbnailNavigPixmap::WBSceneThumbnailNavigPixmap(const QPixmap& pix, WBDocumentProxy* proxy, int pSceneIndex)
    : WBSceneThumbnailPixmap(pix, proxy, pSceneIndex)
    , mEditable(false)
{
    if(0 <= WBDocumentContainer::pageFromSceneIndex(pSceneIndex)){
        setAcceptHoverEvents(true);
        setFlag(QGraphicsItem::ItemIsSelectable, true);
    }
}

WBSceneThumbnailNavigPixmap::~WBSceneThumbnailNavigPixmap()
{

}

void WBSceneThumbnailNavigPixmap::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    event->accept();
    showUI();
    update();
}

void WBSceneThumbnailNavigPixmap::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    event->accept();
    hideUI();
    update();
}

void WBSceneThumbnailNavigPixmap::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    WBSceneThumbnailPixmap::paint(painter, option, widget);
    using namespace WBThumbnailUI;

    if (editable())
    {
        if(deletable())
            draw(painter, *getIcon("close"));
        else
            draw(painter, *getIcon("closeDisabled"));

        draw(painter, *getIcon("duplicate"));
    }
}

void WBSceneThumbnailNavigPixmap::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QPointF p = event->pos();

    using namespace WBThumbnailUI;

    if (triggered(p.y()))
    {
        if(deletable() && getIcon("close")->triggered(p.x()))
        {
            event->accept();
            deletePage();
        }
        else if(getIcon("duplicate")->triggered(p.x()))
        {
            event->accept();
            duplicatePage();
        }
        else
        {
            event->ignore();
        }
        /*
        else if(movableUp() && getIcon("moveUp")->triggered(p.x()))
        {
            event->accept();
            moveUpPage();
        }
        else if (movableDown() && getIcon("moveDown")->triggered(p.x()))
        {
            event->accept();
            moveDownPage();
        }
        */
    }
    else
    {
        event->ignore();
    }
}

void WBSceneThumbnailNavigPixmap::deletePage()
{
    if(WBApplication::mainWindow->yesNoQuestion(QObject::tr("Remove Page"), QObject::tr("Are you sure you want to remove 1 page from the selected document '%0'?").arg(proxy()->metaData(WBSettings::documentName).toString()))){
        WBApplication::boardController->deleteScene(sceneIndex());
    }
}

void WBSceneThumbnailNavigPixmap::duplicatePage()
{
    WBApplication::boardController->duplicateScene(sceneIndex());
}

void WBSceneThumbnailNavigPixmap::moveUpPage()
{
    if (sceneIndex()!=0)
        WBApplication::boardController->moveSceneToIndex(sceneIndex(), sceneIndex() - 1);
}

void WBSceneThumbnailNavigPixmap::moveDownPage()
{
    if (sceneIndex() < WBApplication::boardController->selectedDocument()->pageCount()-1)
        WBApplication::boardController->moveSceneToIndex(sceneIndex(), sceneIndex() + 1);
}


void WBImgTextThumbnailElement::Place(int row, int col, qreal width, qreal height)
{
    int labelSpacing = 0;
    if(this->caption)
    {
        QFontMetrics fm(this->caption->font());
        labelSpacing = WBSettings::thumbnailSpacing + fm.height();
    }
    if(this->thumbnail)
    {
        int w = this->thumbnail->boundingRect().width();
        int h = this->thumbnail->boundingRect().height();

        qreal scaleWidth = width / w;
        qreal scaleHeight = height / h;
        qreal scaleFactor = qMin(scaleWidth, scaleHeight);
        WBThumbnail* pix = dynamic_cast<WBThumbnail*>(this->thumbnail);

        QTransform transform;
        transform.scale(scaleFactor, scaleFactor);

        // Apply the scaling
        this->thumbnail->setTransform(transform);
        this->thumbnail->setFlag(QGraphicsItem::ItemIsSelectable, true);

        if(pix)
        {
            pix->setColumn(col);
            pix->setRow(row);
        }

        QPointF pos((width - w * scaleFactor) / 2,
                    row * (height + labelSpacing) + (height - h * scaleFactor) / 2);

        /*QPointF pos(border + (width - w * scaleFactor) / 2 + col * (width + border),
                    border + row * (height + border + labelSpacing) + (height - h * scaleFactor) / 2);*/

        this->thumbnail->setPos(pos);

        if(this->caption)
        {
            QFontMetrics fm(this->caption->font());
            QString elidedText = fm.elidedText(this->caption->toPlainText(), Qt::ElideRight, width);

            this->caption->setPlainText(elidedText);
            this->caption->setWidth(fm.width(elidedText) + 2 * this->caption->document()->documentMargin());
            pos.setY(pos.y() + (height + h * scaleFactor) / 2 + 5); // What is this 5 ??
            qreal labelWidth = fm.width(elidedText);
            pos.setX((width - labelWidth) / 2 + col * (width + border));
            this->caption->setPos(pos);
        }
    }
}

void WBDraggableThumbnail::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QGraphicsProxyWidget::paint(painter, option, widget);
    using namespace WBThumbnailUI;

    if (editable())
    {        
        if(deletable())
            draw(painter, *getIcon("close"));
        else
            draw(painter, *getIcon("closeDisabled"));

        draw(painter, *getIcon("duplicate"));

        /*
        if(movableUp())
            draw(painter, *getIcon("moveUp"));
        else
            draw(painter, *getIcon("moveUpDisabled"));

        if(movableDown())
            draw(painter, *getIcon("moveDown"));
        else
            draw(painter, *getIcon("moveDownDisabled"));
        */

    }
}

void WBDraggableThumbnail::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    event->accept();
    showUI();
    update();
}

void WBDraggableThumbnail::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    event->accept();
    hideUI();
    update();
}

void WBDraggableThumbnail::deletePage()
{
    if(WBApplication::mainWindow->yesNoQuestion(QObject::tr("Remove Page"), QObject::tr("Are you sure you want to remove 1 page from the selected document '%0'?").arg(WBApplication::documentController->selectedDocument()->metaData(WBSettings::documentName).toString()))){
        WBApplication::boardController->deleteScene(sceneIndex());
    }
}

void WBDraggableThumbnail::duplicatePage()
{
    WBApplication::boardController->duplicateScene(sceneIndex());
}

void WBDraggableThumbnail::moveUpPage()
{
    if (sceneIndex()!=0)
        WBApplication::boardController->moveSceneToIndex(sceneIndex(), sceneIndex() - 1);
}

void WBDraggableThumbnail::moveDownPage()
{
    if (sceneIndex() < WBApplication::boardController->selectedDocument()->pageCount()-1)
        WBApplication::boardController->moveSceneToIndex(sceneIndex(), sceneIndex() + 1);
}

void WBDraggableThumbnail::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QPointF p = event->pos();

    using namespace WBThumbnailUI;

    if (triggered(p.y()))
    {
        if(deletable() && getIcon("close")->triggered(p.x()))
        {
            event->accept();
            deletePage();
        }
        else if(getIcon("duplicate")->triggered(p.x()))
        {
            event->accept();
            duplicatePage();
        }
        else
        {
            event->ignore();
        }
        /*
        else if(movableUp() && getIcon("moveUp")->triggered(p.x()))
            moveUpPage();
        else if (movableDown() && getIcon("moveDown")->triggered(p.x()))
            moveDownPage();*/
    }
    else
    {
        event->ignore();
    }
}

void WBDraggableThumbnail::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    event->accept();
}


void WBDraggableThumbnail::updatePos(qreal width, qreal height)
{
    QFontMetrics fm(mPageNumber->font());
    int labelSpacing = WBSettings::thumbnailSpacing + fm.height();

    int w = boundingRect().width();
    int h = boundingRect().height();

    qreal scaledWidth = width / w;
    qreal scaledHeight = height / h;
    qreal scaledFactor = qMin(scaledWidth, scaledHeight);

    QTransform transform;
    transform.scale(scaledFactor, scaledFactor);

    // Apply the scaling
    setTransform(transform);
    setFlag(QGraphicsItem::ItemIsSelectable, true);

    QPointF position((width - w * scaledFactor) / 2,
                sceneIndex() * (height + labelSpacing) + (height - h * scaledFactor) / 2);

    setPos(position);

    position.setY(position.y() + (height + h * scaledFactor) / 2);
    position.setX(position.x() + (w * scaledFactor - fm.width(mPageNumber->toPlainText())) / 2);

    mPageNumber->setPos(position);
}

void WBDraggableThumbnailPixmap::updatePos(qreal width, qreal height)
{
    QFontMetrics fm(mPageNumber->font());
    int labelSpacing = WBSettings::thumbnailSpacing + fm.height();

    int w = thumbnailPixmap()->boundingRect().width();
    int h = thumbnailPixmap()->boundingRect().height();

    qreal scaledWidth = width / w;
    qreal scaledHeight = height / h;
    qreal scaledFactor = qMin(scaledWidth, scaledHeight);

    QTransform transform;
    transform.scale(scaledFactor, scaledFactor);

    // Apply the scaling
    thumbnailPixmap()->setTransform(transform);
    thumbnailPixmap()->setFlag(QGraphicsItem::ItemIsSelectable, true);

    QPointF position((width - w * scaledFactor) / 2,
                sceneIndex() * (height + labelSpacing) + (height - h * scaledFactor) / 2);

    thumbnailPixmap()->setPos(position);

    position.setY(position.y() + (height + h * scaledFactor) / 2);
    position.setX(position.x() + (w * scaledFactor - fm.width(mPageNumber->toPlainText())) / 2);

    mPageNumber->setPos(position);
}


WBThumbnailUI::WBThumbnailUIIcon* WBThumbnailUI::addIcon(const QString& thumbnailIcon, int pos)
{
    QString thumbnailIconPath = ":images/" + thumbnailIcon + ".svg";
    WBThumbnailUIIcon* newIcon = new WBThumbnailUIIcon(thumbnailIconPath, pos);

    using namespace WBThumbnailUI::_private;
    if (!newIcon)
        qDebug() << "cannot add Icon : check path : " + thumbnailIconPath;
    else
        catalog.insert(thumbnailIcon, newIcon);

    return newIcon;
}

WBThumbnailUI::WBThumbnailUIIcon* WBThumbnailUI::getIcon(const QString& thumbnailIcon)
{
    using namespace WBThumbnailUI::_private;
    if (!catalog.contains(thumbnailIcon))
        qDebug() << "cannot get Icon: check path ':images/" + thumbnailIcon + ".svg'";

    return catalog.value(thumbnailIcon, NULL);
}

void WBThumbnailUI::draw(QPainter *painter, const WBThumbnailUIIcon &thumbnailIcon)
{
    using namespace WBThumbnailUI;
    painter->drawPixmap(thumbnailIcon.pos() * (ICONSIZE + ICONSPACING), 0, ICONSIZE, ICONSIZE, thumbnailIcon);
}

void WBThumbnailUI::_private::initCatalog()
{
    using namespace WBThumbnailUI;
    using namespace WBThumbnailUI::_private;

    addIcon("close", 0);
    addIcon("closeDisabled", 0);

    addIcon("duplicate", 1);

    addIcon("moveUp", 2);
    addIcon("moveUpDisabled", 2);

    addIcon("moveDown", 3);
    addIcon("moveDownDisabled", 3);
}

bool WBThumbnailUI::triggered(qreal y)
{
    return (y >= 0 && y <= WBThumbnailUI::ICONSIZE);
}
