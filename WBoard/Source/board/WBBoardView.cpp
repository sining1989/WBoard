#include "WBBoardView.h"

#include <QtWidgets>
#include <QtXml>
#include <QListView>

#include "WBDrawingController.h"

#include "frameworks/WBGeometryUtils.h"
#include "frameworks/WBPlatformUtils.h"

#include "core/WBSettings.h"
#include "core/WBMimeData.h"
#include "core/WBApplication.h"
#include "core/WBSetting.h"
#include "core/WBPersistenceManager.h"
#include "core/WB.h"

#include "network/WBHttpGet.h"

#include "gui/WBStylusPalette.h"
#include "gui/WBRubberBand.h"
#include "gui/WBToolWidget.h"
#include "gui/WBResources.h"
#include "gui/WBMainWindow.h"
#include "gui/WBThumbnailWidget.h"

#include "board/WBBoardController.h"
#include "board/WBBoardPaletteManager.h"

#ifdef Q_OS_OSX
#include "core/WBApplicationController.h"
#include "desktop/WBDesktopAnnotationController.h"
#endif

#include "domain/WBGraphicsTextItem.h"
#include "domain/WBGraphicsPixmapItem.h"
#include "domain/WBGraphicsWidgetItem.h"
#include "domain/WBGraphicsPDFItem.h"
#include "domain/WBGraphicsPolygonItem.h"
#include "domain/WBItem.h"
#include "domain/WBGraphicsMediaItem.h"
#include "domain/WBGraphicsSvgItem.h"
#include "domain/WBGraphicsGroupContainerItem.h"
#include "domain/WBGraphicsStrokesGroup.h"
#include "domain/WBGraphicsItemDelegate.h"

#include "document/WBDocumentProxy.h"

#include "tools/WBGraphicsRuler.h"
#include "tools/WBGraphicsCurtainItem.h"
#include "tools/WBGraphicsCompass.h"
#include "tools/WBGraphicsCache.h"
#include "tools/WBGraphicsTriangle.h"
#include "tools/WBGraphicsProtractor.h"

#include "core/memcheck.h"

WBBoardView::WBBoardView (WBBoardController* pController, QWidget* pParent, bool isControl, bool isDesktop)
    : QGraphicsView (pParent)
    , mController (pController)
    , mIsCreatingTextZone (false)
    , mIsCreatingSceneGrabZone (false)
    , mOkOnWidget(false)
    , suspendedMousePressEvent(NULL)
    , mLongPressInterval(1000)
    , mIsDragInProgress(false)
    , mMultipleSelectionIsEnabled(false)
    , bIsControl(isControl)
    , bIsDesktop(isDesktop)
{
    init ();

    mFilterZIndex = false;
    /*
    mFilterZIndex = true;
    mStartLayer = WBItemLayerType::FixedBackground;
    mEndLayer = WBItemLayerType::Control;
    */


    mLongPressTimer.setInterval(mLongPressInterval);
    mLongPressTimer.setSingleShot(true);
}

WBBoardView::WBBoardView (WBBoardController* pController, int pStartLayer, int pEndLayer, QWidget* pParent, bool isControl, bool isDesktop)
    : QGraphicsView (pParent)
    , mController (pController)
    , suspendedMousePressEvent(NULL)
    , mLongPressInterval(1000)
    , mIsDragInProgress(false)
    , mMultipleSelectionIsEnabled(false)
    , bIsControl(isControl)
    , bIsDesktop(isDesktop)
{
    init ();

    mStartLayer = pStartLayer;
    mEndLayer = pEndLayer;

    mFilterZIndex = true;

    mLongPressTimer.setInterval(mLongPressInterval);
    mLongPressTimer.setSingleShot(true);
}

WBBoardView::~WBBoardView ()
{
    if (suspendedMousePressEvent){
        delete suspendedMousePressEvent;
        suspendedMousePressEvent = NULL;
    }
}

void WBBoardView::init ()
{
    connect (WBSettings::settings ()->boardPenPressureSensitive, SIGNAL (changed (QVariant)),
             this, SLOT (settingChanged (QVariant)));

    connect (WBSettings::settings ()->boardMarkerPressureSensitive, SIGNAL (changed (QVariant)),
             this, SLOT (settingChanged (QVariant)));

    connect (WBSettings::settings ()->boardUseHighResTabletEvent, SIGNAL (changed (QVariant)),
             this, SLOT (settingChanged (QVariant)));

    setOptimizationFlags (QGraphicsView::IndirectPainting | QGraphicsView::DontSavePainterState); // enable WBBoardView::drawItems filter
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    setWindowFlags (Qt::FramelessWindowHint);
    setFrameStyle (QFrame::NoFrame);
    setRenderHints (QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);
    setVerticalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
    setAcceptDrops (true);

    mTabletStylusIsPressed = false;
    mMouseButtonIsPressed = false;
    mPendingStylusReleaseEvent = false;

    setCacheMode (QGraphicsView::CacheBackground);

    mUsingTabletEraser = false;
    mIsCreatingTextZone = false;
    mRubberBand = 0;
    mUBRubberBand = 0;

    mVirtualKeyboardActive = false;

    settingChanged (QVariant ());

    unsetCursor();

    movingItem = NULL;
    mWidgetMoved = false;
}

WBGraphicsScene* WBBoardView::scene ()
{
    return qobject_cast<WBGraphicsScene*> (QGraphicsView::scene ());
}

void WBBoardView::keyPressEvent (QKeyEvent *event)
{
    QApplication::sendEvent (scene (), event);

    if (!event->isAccepted ())
    {
        switch (event->key ())
        {
        case Qt::Key_Up:
        case Qt::Key_PageUp:
        case Qt::Key_Left:
        {
            mController->previousScene ();
            break;
        }

        case Qt::Key_Down:
        case Qt::Key_PageDown:
        case Qt::Key_Right:
        case Qt::Key_Space:
        {
            mController->nextScene ();
            break;
        }

        case Qt::Key_Home:
        {
            mController->firstScene ();
            break;
        }
        case Qt::Key_End:
        {
            mController->lastScene ();
            break;
        }
        case Qt::Key_Insert:
        {
            mController->addScene ();
            break;
        }
        case Qt::Key_Control:
        case Qt::Key_Shift:
        {
            setMultiselection(true);
        }break;
        }


        if (event->modifiers () & Qt::ControlModifier) // keep only ctrl/cmd keys
        {
            switch (event->key ())
            {
            case Qt::Key_Plus:
            case Qt::Key_I:
            {
                mController->zoomIn ();
                event->accept ();
                break;
            }
            case Qt::Key_Minus:
            case Qt::Key_O:
            {
                mController->zoomOut ();
                event->accept ();
                break;
            }
            case Qt::Key_0:
            {
                mController->zoomRestore ();
                event->accept ();
                break;
            }
            case Qt::Key_Left:
            {
                mController->handScroll (-100, 0);
                event->accept ();
                break;
            }
            case Qt::Key_Right:
            {
                mController->handScroll (100, 0);
                event->accept ();
                break;
            }
            case Qt::Key_Up:
            {
                mController->handScroll (0, -100);
                event->accept ();
                break;
            }
            case Qt::Key_Down:
            {
                mController->handScroll (0, 100);
                event->accept ();
                break;
            }
            default:
            {
                // NOOP
            }
            }
        }
    }

    if (event->isAccepted())
        setMultiselection(false);
}


void WBBoardView::keyReleaseEvent(QKeyEvent *event)
{

    if (Qt::Key_Shift == event->key() ||Qt::Key_Control == event->key())
        setMultiselection(false);

    QGraphicsView::keyReleaseEvent(event);
}

bool WBBoardView::event (QEvent * e)
{
    if (e->type() == QEvent::Gesture)
    {
        QGestureEvent *gestureEvent = dynamic_cast<QGestureEvent *> (e);
        if (gestureEvent)
        {
            QSwipeGesture* swipe = dynamic_cast<QSwipeGesture*> (gestureEvent->gesture (Qt::SwipeGesture));
            if (swipe)
            {
                if (swipe->horizontalDirection () == QSwipeGesture::Left)
                {
                    mController->previousScene ();
                    gestureEvent->setAccepted (swipe, true);
                }

                if (swipe->horizontalDirection () == QSwipeGesture::Right)
                {
                    mController->nextScene ();
                    gestureEvent->setAccepted (swipe, true);
                }
            }
        }
    }

    return QGraphicsView::event (e);
}

void WBBoardView::tabletEvent (QTabletEvent * event)
{
    if (!mUseHighResTabletEvent) {
        event->setAccepted (false);
        return;
    }

    WBDrawingController *dc = WBDrawingController::drawingController ();

    QPointF tabletPos = event->pos();
    WBStylusTool::Enum currentTool = (WBStylusTool::Enum)dc->stylusTool ();

    if (event->type() == QEvent::TabletPress || event->type() == QEvent::TabletEnterProximity) {
        if (event->pointerType () == QTabletEvent::Eraser) {
            dc->setStylusTool (WBStylusTool::Eraser);
            mUsingTabletEraser = true;
        }
        else {
            if (mUsingTabletEraser && currentTool == WBStylusTool::Eraser)
                dc->setStylusTool (dc->latestDrawingTool ());

            mUsingTabletEraser = false;
        }
    }

    QPointF scenePos = viewportTransform ().inverted ().map (tabletPos);

    qreal pressure = 1.0;
    if (((currentTool == WBStylusTool::Pen || currentTool == WBStylusTool::Line) && mPenPressureSensitive) ||
            (currentTool == WBStylusTool::Marker && mMarkerPressureSensitive))
        pressure = event->pressure ();
    else{
        event->setAccepted (false);
        return;
    }

    bool acceptEvent = true;
#ifdef Q_OS_OSX
    //Work around #1388. After selecting annotation tool in desktop mode, annotation view appears on top when
    //using Mac OS X. In this case tablet event should send mouse event so as to let user interact with
    //stylus palette.
    Q_ASSERT(WBApplication::applicationController->uninotesController());
    if (WBApplication::applicationController->uninotesController()->drawingView() == this) {
        if (WBApplication::applicationController->uninotesController()->desktopPalettePath().contains(event->pos())) {
            acceptEvent = false;
        }
    }
#endif

    switch (event->type()) {
		case QEvent::TabletPress: {
			mTabletStylusIsPressed = true;
			scene()->inputDevicePress (scenePos, pressure);

			break;
		}
		case QEvent::TabletMove: {
			if (mTabletStylusIsPressed)
				scene ()->inputDeviceMove (scenePos, pressure);

			acceptEvent = false;

			break;

		}
		case QEvent::TabletRelease: {
			WBStylusTool::Enum currentTool = (WBStylusTool::Enum)dc->stylusTool ();
			scene()->setToolCursor (currentTool);
			setToolCursor (currentTool);

			scene()->inputDeviceRelease ();

			mPendingStylusReleaseEvent = false;

			mTabletStylusIsPressed = false;
			mMouseButtonIsPressed = false;

			break;
		}
		default: {
			//NOOP
		}
    }


    event->setAccepted (acceptEvent);

}

bool WBBoardView::itemIsLocked(QGraphicsItem *item)
{
    if (!item)
        return false;

    return item->data(WBGraphicsItemData::ItemLocked).toBool();
}

bool WBBoardView::itemHaveParentWithType(QGraphicsItem *item, int type)
{
    if (!item)
        return false;

    if (type == item->type())
        return true;

    return itemHaveParentWithType(item->parentItem(), type);

}

bool WBBoardView::isWBItem(QGraphicsItem *item)
{
    if ((WBGraphicsItemType::UserTypesCount > item->type()) && (item->type() > QGraphicsItem::UserType))
        return true;

    return false;
}

bool WBBoardView::isCppTool(QGraphicsItem *item)
{
    return (item->type() == WBGraphicsItemType::CompassItemType
            || item->type() == WBGraphicsItemType::RulerItemType
            || item->type() == WBGraphicsItemType::ProtractorItemType
            || item->type() == WBGraphicsItemType::TriangleItemType
            || item->type() == WBGraphicsItemType::CurtainItemType);
}

void WBBoardView::handleItemsSelection(QGraphicsItem *item)
{
    if (item)
    {
         if(item->parentItem() && WBGraphicsGroupContainerItem::Type == movingItem->parentItem()->type())
            return;

        if (DelegateButton::Type == item->type())
            return;

        if (QGraphicsSvgItem::Type == item->type())
            return;

        if (WBGraphicsDelegateFrame::Type == item->type())
            return;

        if (!isMultipleSelectionEnabled())
        {
            if ((WBGraphicsItemType::UserTypesCount > item->type()) && (item->type() > QGraphicsItem::UserType))
            {
                scene()->deselectAllItemsExcept(item);
            }
        }
    }
}

bool WBBoardView::itemShouldReceiveMousePressEvent(QGraphicsItem *item)
{
    if (!item)
        return true;

    if (item == scene()->backgroundObject())
        return false;

    WBStylusTool::Enum currentTool = (WBStylusTool::Enum)WBDrawingController::drawingController()->stylusTool();

    switch(item->type())
    {
    case WBGraphicsProtractor::Type:
    case WBGraphicsRuler::Type:
    case WBGraphicsTriangle::Type:
    case WBGraphicsCompass::Type:
    case WBGraphicsCache::Type:
        return true;
    case WBGraphicsDelegateFrame::Type:
        if (currentTool == WBStylusTool::Play)
            return false;
        return true;
    case WBGraphicsPixmapItem::Type:
    case WBGraphicsSvgItem::Type:
        if (currentTool == WBStylusTool::Play)
            return true;
        if (item->isSelected())
            return true;
        else
            return false;
    case DelegateButton::Type:
        return true;

    case WBGraphicsMediaItem::Type:
    case WBGraphicsVideoItem::Type:
    case WBGraphicsAudioItem::Type:
        return false;

    case WBGraphicsTextItem::Type:
        if (currentTool == WBStylusTool::Play)
            return true;
        if ((currentTool == WBStylusTool::Selector) && item->isSelected())
            return true;
        if ((currentTool == WBStylusTool::Selector) && item->parentItem() && item->parentItem()->isSelected())
            return true;
        if (currentTool != WBStylusTool::Selector)
            return false;
        break;

    case WBGraphicsItemType::StrokeItemType:
        if (currentTool == WBStylusTool::Play || currentTool == WBStylusTool::Selector)
            return true;
        break;

    case WBGraphicsGroupContainerItem::Type:
        if(currentTool == WBStylusTool::Play)
        {
            movingItem = NULL;
            return true;
        }
        return false;
        break;
	//case QWebEngineView::Type:
 //       return true;
	case QGraphicsProxyWidget::Type:
        return false;

    case WBGraphicsWidgetItem::Type:
        if (currentTool == WBStylusTool::Selector && item->parentItem() && item->parentItem()->isSelected())
            return true;
        if (currentTool == WBStylusTool::Selector && item->isSelected())
            return true;
        if (currentTool == WBStylusTool::Play)
            return true;
        return false;
        break;
    }

    return !isWBItem(item);
}

bool WBBoardView::itemShouldReceiveSuspendedMousePressEvent(QGraphicsItem *item)
{
    if (!item)
        return false;

    if (item == scene()->backgroundObject())
        return false;

    WBStylusTool::Enum currentTool = (WBStylusTool::Enum)WBDrawingController::drawingController()->stylusTool();

    switch(item->type())
    {
    //case QWebEngineView::Type:
    //    return false;
    case WBGraphicsPixmapItem::Type:
    case WBGraphicsSvgItem::Type:
    case WBGraphicsTextItem::Type:
    case WBGraphicsWidgetItem::Type:
        if (currentTool == WBStylusTool::Selector && !item->isSelected() && item->parentItem())
            return true;
        if (currentTool == WBStylusTool::Selector && item->isSelected())
            return true;
        break;

    case DelegateButton::Type:
    case WBGraphicsMediaItem::Type:
    case WBGraphicsVideoItem::Type:
    case WBGraphicsAudioItem::Type:
        return true;
    }

    return false;

}

bool WBBoardView::itemShouldBeMoved(QGraphicsItem *item)
{
    if (!item)
        return false;

    if (item == scene()->backgroundObject())
        return false;

    if (!(mMouseButtonIsPressed || mTabletStylusIsPressed))
        return false;

    if (movingItem->data(WBGraphicsItemData::ItemLocked).toBool())
        return false;

    if (movingItem->parentItem() && WBGraphicsGroupContainerItem::Type == movingItem->parentItem()->type() && !movingItem->isSelected() && movingItem->parentItem()->isSelected())
        return false;

    WBStylusTool::Enum currentTool = (WBStylusTool::Enum)WBDrawingController::drawingController()->stylusTool();

    switch(item->type())
    {
    case WBGraphicsCurtainItem::Type:
    case WBGraphicsGroupContainerItem::Type:
        return true;

    case WBGraphicsWidgetItem::Type:
        if(currentTool == WBStylusTool::Selector && item->isSelected())
            return false;
        if(currentTool == WBStylusTool::Play)
            return false;

    case WBGraphicsSvgItem::Type:
    case WBGraphicsPixmapItem::Type:
        if (currentTool == WBStylusTool::Play || !item->isSelected())
            return true;
        if (item->isSelected())
            return false;
    case WBGraphicsMediaItem::Type:
    case WBGraphicsVideoItem::Type:
    case WBGraphicsAudioItem::Type:
        return true;
    case WBGraphicsStrokesGroup::Type:
        return false;
    case WBGraphicsTextItem::Type:
        return !item->isSelected();
    }

    return false;
}


QGraphicsItem* WBBoardView::determineItemToPress(QGraphicsItem *item)
{
    if(item)
    {
        WBStylusTool::Enum currentTool = (WBStylusTool::Enum)WBDrawingController::drawingController()->stylusTool();

        if (WBStylusTool::Selector == currentTool
                && item->parentItem()
                && WBGraphicsGroupContainerItem::Type == item->parentItem()->type()
                && !item->parentItem()->isSelected())
            return item->parentItem();

        if(item->parentItem() && WBGraphicsStrokesGroup::Type == item->parentItem()->type())
            return determineItemToPress(item->parentItem());
    }

    return item;
}

QGraphicsItem* WBBoardView::determineItemToMove(QGraphicsItem *item)
{
    if(item)
    {
        WBStylusTool::Enum currentTool = (WBStylusTool::Enum)WBDrawingController::drawingController()->stylusTool();

        if ((WBStylusTool::Play == currentTool) && (WBGraphicsWidgetItem::Type == item->type()))
            return item;

        if(item->parentItem() && WBGraphicsGroupContainerItem::Type == item->parentItem()->type())
        {
            if (WBStylusTool::Play == currentTool && item->parentItem()->isSelected())
                return item->parentItem();

            if (WBGraphicsStrokesGroup::Type == item->type())
                return item->parentItem();

            if (item->parentItem()->isSelected())
                return item;

            if (item->isSelected())
                return NULL;

            return item->parentItem();
        }

        if(item->parentItem() && WBGraphicsStrokesGroup::Type == item->parentItem()->type())
            return determineItemToMove(item->parentItem());
    }

    return item;
}

void WBBoardView::handleItemMousePress(QMouseEvent *event)
{
    mLastPressedMousePos = mapToScene(event->pos());

    movingItem = determineItemToPress(movingItem);
    handleItemsSelection(movingItem);

    if (isMultipleSelectionEnabled())
        return;

    if (itemShouldReceiveMousePressEvent(movingItem)){
        QGraphicsView::mousePressEvent (event);

        QGraphicsItem* item = determineItemToPress(scene()->itemAt(this->mapToScene(event->localPos().toPoint()), transform()));
  
        if (item && (item->type() == QGraphicsProxyWidget::Type) && item->parentObject() && item->parentObject()->type() != QGraphicsProxyWidget::Type)
        {
            QList<QGraphicsItem*> children = item->childItems();

            for( QList<QGraphicsItem*>::iterator it = children.begin(); it != children.end(); ++it )
                if ((*it)->pos().x() < 0 || (*it)->pos().y() < 0)
                    (*it)->setPos(0,item->boundingRect().size().height());
        }
    }
    else
    {
        if (movingItem)
        {
            WBGraphicsItem *graphicsItem = dynamic_cast<WBGraphicsItem*>(movingItem);
            if (graphicsItem)
                graphicsItem->Delegate()->startUndoStep();

            movingItem->clearFocus();
        }

        if (suspendedMousePressEvent)
        {
            delete suspendedMousePressEvent;
            suspendedMousePressEvent = NULL;
        }

        if (itemShouldReceiveSuspendedMousePressEvent(movingItem))
        {
            suspendedMousePressEvent = new QMouseEvent(event->type(), event->pos(), event->button(), event->buttons(), event->modifiers());
        }
    }
}

void WBBoardView::handleItemMouseMove(QMouseEvent *event)
{
    movingItem = determineItemToMove(movingItem);

    if (movingItem && itemShouldBeMoved(movingItem) && (mMouseButtonIsPressed || mTabletStylusIsPressed))
    {
        QPointF scenePos = mapToScene(event->pos());
        QPointF newPos = movingItem->pos() + scenePos - mLastPressedMousePos;
        movingItem->setPos(newPos);
        mLastPressedMousePos = scenePos;
        mWidgetMoved = true;
        event->accept();
    }
    else
    {
        QPointF posBeforeMove;
        QPointF posAfterMove;

        if (movingItem)
            posBeforeMove = movingItem->pos();

        QGraphicsView::mouseMoveEvent (event);

        if (movingItem)
            posAfterMove = movingItem->pos();

        mWidgetMoved = ((posAfterMove-posBeforeMove).manhattanLength() != 0);

        if (movingItem && mWidgetMoved && WBGraphicsW3CWidgetItem::Type == movingItem->type())
            movingItem->setPos(posBeforeMove);
    }
}

void WBBoardView::rubberItems()
{
    if (mUBRubberBand)
        mRubberedItems = items(mUBRubberBand->geometry());

    foreach(QGraphicsItem *item, mRubberedItems)
    {
        if (item->parentItem() && WBGraphicsGroupContainerItem::Type == item->parentItem()->type())
            mRubberedItems.removeOne(item);
    }
}

void WBBoardView::moveRubberedItems(QPointF movingVector)
{
    QRectF invalidateRect = scene()->itemsBoundingRect();

    foreach (QGraphicsItem *item, mRubberedItems)
    {

        if (item->type() == WBGraphicsW3CWidgetItem::Type
                || item->type() == WBGraphicsPixmapItem::Type
                || item->type() == WBGraphicsMediaItem::Type
                || item->type() == WBGraphicsVideoItem::Type
                || item->type() == WBGraphicsAudioItem::Type
                || item->type() == WBGraphicsSvgItem::Type
                || item->type() == WBGraphicsTextItem::Type
                || item->type() == WBGraphicsStrokesGroup::Type
                || item->type() == WBGraphicsGroupContainerItem::Type)
        {
            item->setPos(item->pos()+movingVector);
        }
    }

    scene()->invalidate(invalidateRect);
}

void WBBoardView::setMultiselection(bool enable)
{
    mMultipleSelectionIsEnabled = enable;
}

// work around for handling tablet events on MAC OS with Qt 4.8.0 and above
#if defined(Q_OS_OSX)
bool WBBoardView::directTabletEvent(QEvent *event)
{
    QTabletEvent *tEvent = static_cast<QTabletEvent *>(event);
    tEvent = new QTabletEvent(tEvent->type()
                              , mapFromGlobal(tEvent->pos())
                              , tEvent->globalPos()
                              , tEvent->device()
                              , tEvent->pointerType()
                              , tEvent->pressure()
                              , tEvent->xTilt()
                              , tEvent->yTilt()
                              , tEvent->tangentialPressure()
                              , tEvent->rotation()
                              , tEvent->z()
                              , tEvent->modifiers()
                              , tEvent->uniqueId());

    if (geometry().contains(tEvent->pos()))
    {
        if (NULL == widgetForTabletEvent(this->parentWidget(), tEvent->pos()))
        {
            tabletEvent(tEvent);
            return true;
        }
    }
    return false;
}

QWidget *WBBoardView::widgetForTabletEvent(QWidget *w, const QPoint &pos)
{
    Q_ASSERT(w);

    // it should work that, but it doesn't. So we check if it is control view.
    //WBBoardView *board = qobject_cast<WBBoardView *>(w);
    WBBoardView *board = WBApplication::boardController->controlView();

    QWidget *childAtPos = NULL;

    QList<QObject *> childs = w->children();
    foreach(QObject *child, childs)
    {
        QWidget *childWidget = qobject_cast<QWidget *>(child);
        if (childWidget)
        {
            if (childWidget->isVisible() && childWidget->geometry().contains(pos))
            {
                QWidget *lastChild = widgetForTabletEvent(childWidget, pos);

                if (board && board->viewport() == lastChild)
                    continue;

                if (NULL != lastChild)
                    childAtPos = lastChild;
                else
                    childAtPos = childWidget;

                break;
            }
            else
                childAtPos = NULL;
        }
    }
    return childAtPos;
}
#endif

void WBBoardView::longPressEvent()
{
    WBDrawingController *drawingController = WBDrawingController::drawingController();
    WBStylusTool::Enum currentTool = (WBStylusTool::Enum)WBDrawingController::drawingController ()->stylusTool ();

    disconnect(&mLongPressTimer, SIGNAL(timeout()), this, SLOT(longPressEvent()));

    if (WBStylusTool::Selector == currentTool)
    {
        drawingController->setStylusTool(WBStylusTool::Play);
    }
    else
        if (currentTool == WBStylusTool::Play)
        {
            drawingController->setStylusTool(WBStylusTool::Selector);
        }
        else
            if (WBStylusTool::Eraser == currentTool)
            {
                WBApplication::boardController->paletteManager()->toggleErasePalette(true);
            }

}

void WBBoardView::mousePressEvent (QMouseEvent *event)
{
    if (!bIsControl && !bIsDesktop) {
        event->ignore();
        return;
    }

    mIsDragInProgress = false;

    if (isAbsurdPoint (event->pos ())) {
        event->accept ();
        return;
    }

    mMouseDownPos = event->pos ();
    movingItem = scene()->itemAt(this->mapToScene(event->localPos().toPoint()), QTransform());

    if (event->button () == Qt::LeftButton && isInteractive())
    {
        int currentTool = (WBStylusTool::Enum)WBDrawingController::drawingController ()->stylusTool ();
        if (!mTabletStylusIsPressed)
            mMouseButtonIsPressed = true;

        switch (currentTool) {
        case WBStylusTool::ZoomIn :
            mController->zoomIn (mapToScene (event->pos ()));
            event->accept();
            break;

        case WBStylusTool::ZoomOut :
            mController->zoomOut (mapToScene (event->pos ()));
            event->accept();
            break;

        case WBStylusTool::Hand :
            viewport()->setCursor(QCursor (Qt::ClosedHandCursor));
            mPreviousPoint = event->localPos();
            event->accept();
            break;

        case WBStylusTool::Selector :
        case WBStylusTool::Play :
            if (bIsDesktop) {
                event->ignore();
                return;
            }

            if (scene()->backgroundObject() == movingItem)
                movingItem = NULL;

            connect(&mLongPressTimer, SIGNAL(timeout()), this, SLOT(longPressEvent()));
            if (!movingItem && !mController->cacheIsVisible())
                mLongPressTimer.start();

            handleItemMousePress(event);
            event->accept();
            break;

        case WBStylusTool::Text : {
            int frameWidth = WBSettings::settings ()->objectFrameWidth;
            QRectF fuzzyRect (0, 0, frameWidth * 4, frameWidth * 4);
            fuzzyRect.moveCenter (mapToScene (mMouseDownPos));

            WBGraphicsTextItem* foundTextItem = 0;
            QListIterator<QGraphicsItem *> it (scene ()->items (fuzzyRect));

            while (it.hasNext () && !foundTextItem)
                foundTextItem = qgraphicsitem_cast<WBGraphicsTextItem*>(it.next ());

            if (foundTextItem)
            {
                mIsCreatingTextZone = false;
                QGraphicsView::mousePressEvent (event);
            }
            else
            {
                scene()->deselectAllItems();

                if (!mRubberBand)
                    mRubberBand = new WBRubberBand (QRubberBand::Rectangle, this);
                mRubberBand->setGeometry (QRect (mMouseDownPos, QSize ()));
                mRubberBand->show();
                mIsCreatingTextZone = true;

                event->accept ();
            }
        } break;

        case WBStylusTool::Capture :
            scene ()->deselectAllItems ();

            if (!mRubberBand)
                mRubberBand = new WBRubberBand (QRubberBand::Rectangle, this);

            mRubberBand->setGeometry (QRect (mMouseDownPos, QSize ()));
            mRubberBand->show ();
            mIsCreatingSceneGrabZone = true;

            event->accept ();
            break;

        default:
            if(WBDrawingController::drawingController()->mActiveRuler==NULL) {
                viewport()->setCursor (QCursor (Qt::BlankCursor));
            }
            if (scene () && !mTabletStylusIsPressed) {
                if (currentTool == WBStylusTool::Eraser) {
                    connect(&mLongPressTimer, SIGNAL(timeout()), this, SLOT(longPressEvent()));
                    mLongPressTimer.start();
                }
                scene()->inputDevicePress(mapToScene(WBGeometryUtils::pointConstrainedInRect(event->pos(), rect())));
            }
            event->accept ();
        }
    }
}


void WBBoardView::mouseMoveEvent (QMouseEvent *event)
{
    //    static QTime lastCallTime;
    //    if (!lastCallTime.isNull()) {
    //        qDebug() << "time interval is " << lastCallTime.msecsTo(QTime::currentTime());
    //    }

    //  QTime mouseMoveTime = QTime::currentTime();
    if(!mIsDragInProgress && ((mapToScene(event->pos()) - mLastPressedMousePos).manhattanLength() < QApplication::startDragDistance())) {
        return;
    }

    mIsDragInProgress = true;
    mWidgetMoved = true;
    mLongPressTimer.stop();

    if (isAbsurdPoint (event->pos ())) {
        event->accept ();
        return;
    }

    if ((WBDrawingController::drawingController()->isDrawingTool()) && !mMouseButtonIsPressed)
        QGraphicsView::mouseMoveEvent(event);

    int currentTool = static_cast<int>(WBDrawingController::drawingController()->stylusTool());
    switch (currentTool) {

    case WBStylusTool::Hand : {
        if (!mMouseButtonIsPressed && !mTabletStylusIsPressed) {
            break;
        }
        QPointF eventPosition = event->localPos();
        qreal dx = eventPosition.x () - mPreviousPoint.x ();
        qreal dy = eventPosition.y () - mPreviousPoint.y ();
        mController->handScroll (dx, dy);
        mPreviousPoint = eventPosition;
        event->accept ();
    } break;

    case WBStylusTool::Selector :
    case WBStylusTool::Play : {
        if (bIsDesktop) {
            event->ignore();
            return;
        }

        bool rubberMove = (currentTool != (WBStylusTool::Play))
                && (mMouseButtonIsPressed || mTabletStylusIsPressed)
                && !movingItem;

        if (rubberMove) {
            QRect bandRect(mMouseDownPos, event->pos());

            bandRect = bandRect.normalized();

            if (!mUBRubberBand) {
                mUBRubberBand = new WBRubberBand(QRubberBand::Rectangle, this);
            }
            mUBRubberBand->setGeometry(bandRect);
            mUBRubberBand->show();

            //          QTime startTime = QTime::currentTime();
            //          QTime testTime = QTime::currentTime();
            QList<QGraphicsItem *> rubberItems = items(bandRect);
            //          qDebug() << "==================";
            //          qDebug() << "| ====rubber items" << testTime.msecsTo(QTime::currentTime());
            //          testTime = QTime::currentTime();
            foreach (QGraphicsItem *item, mJustSelectedItems) {
                if (!rubberItems.contains(item)) {
                    item->setSelected(false);
                    mJustSelectedItems.remove(item);
                }
            }
            //          qDebug() << "| ===foreach length" << testTime.msecsTo(QTime::currentTime());
            //          testTime = QTime::currentTime();

            int counter = 0;
            if (currentTool == WBStylusTool::Selector) {
                foreach (QGraphicsItem *item, items(bandRect)) {

                    if(item->type() == WBGraphicsItemType::PolygonItemType && item->parentItem())
                        item = item->parentItem();

                    if (item->type() == WBGraphicsW3CWidgetItem::Type
                            || item->type() == WBGraphicsPixmapItem::Type
                            || item->type() == WBGraphicsVideoItem::Type
                            || item->type() == WBGraphicsAudioItem::Type
                            || item->type() == WBGraphicsSvgItem::Type
                            || item->type() == WBGraphicsTextItem::Type
                            || item->type() == WBGraphicsStrokesGroup::Type
                            || item->type() == WBGraphicsGroupContainerItem::Type) {


                        if (!mJustSelectedItems.contains(item)) {
                            counter++;
                            item->setSelected(true);
                            mJustSelectedItems.insert(item);
                        }
                    }
                }
            }

            //          qDebug() << "| ==selected items count" << counter << endl
            //                   << "| ==selection time" << testTime.msecsTo(QTime::currentTime()) << endl
            //                   << "| =elapsed time " << startTime.msecsTo(QTime::currentTime()) << endl
            //                   << "==================";
            //          QCoreApplication::removePostedEvents(scene(), 0);
        }
        handleItemMouseMove(event);
    } break;

    case WBStylusTool::Text :
    case WBStylusTool::Capture : {
        if (mRubberBand && (mIsCreatingTextZone || mIsCreatingSceneGrabZone)) {
            mRubberBand->setGeometry(QRect(mMouseDownPos, event->pos()).normalized());
            event->accept();
        }
        else
            QGraphicsView::mouseMoveEvent (event);

    } break;

    default:
        if (!mTabletStylusIsPressed && scene()) {
            scene()->inputDeviceMove(mapToScene(WBGeometryUtils::pointConstrainedInRect(event->pos(), rect())) , mMouseButtonIsPressed);
        }
        event->accept ();
    }

    //  qDebug() << "mouse move time" << mouseMoveTime.msecsTo(QTime::currentTime());
    //  lastCallTime = QTime::currentTime();

}

void WBBoardView::mouseReleaseEvent (QMouseEvent *event)
{
    WBStylusTool::Enum currentTool = (WBStylusTool::Enum)WBDrawingController::drawingController ()->stylusTool ();

    setToolCursor (currentTool);
    // first/ propagate device release to the scene
    if (scene())
        scene()->inputDeviceRelease();

    if (currentTool == WBStylusTool::Selector)
    {
        if (bIsDesktop) {
            event->ignore();
            return;
        }

        WBGraphicsItem *graphicsItem = dynamic_cast<WBGraphicsItem*>(movingItem);
        if (graphicsItem)
            graphicsItem->Delegate()->commitUndoStep();

        bool bReleaseIsNeed = true;
        if (movingItem != determineItemToPress(scene()->itemAt(this->mapToScene(event->localPos().toPoint()), QTransform())))
        {
            movingItem = NULL;
            bReleaseIsNeed = false;
        }
        if (mWidgetMoved)
        {
            mWidgetMoved = false;
            movingItem = NULL;
        }
        else
            if (movingItem && (!isCppTool(movingItem) || WBGraphicsCurtainItem::Type == movingItem->type()))
            {
                if (suspendedMousePressEvent)
                {
                    QGraphicsView::mousePressEvent(suspendedMousePressEvent);     // suspendedMousePressEvent is deleted by old Qt event loop
                    movingItem = NULL;
                    delete suspendedMousePressEvent;
                    suspendedMousePressEvent = NULL;
                    bReleaseIsNeed = true;
                }
                else
                {
                    if (isWBItem(movingItem) &&
                            DelegateButton::Type != movingItem->type() &&
                            WBGraphicsDelegateFrame::Type !=  movingItem->type() &&
                            WBGraphicsCache::Type != movingItem->type() &&
                            //QGraphicsView::Type != movingItem->type() && // for W3C widgets as Tools.
                            !(!isMultipleSelectionEnabled() && movingItem->parentItem() && WBGraphicsWidgetItem::Type == movingItem->type() && WBGraphicsGroupContainerItem::Type == movingItem->parentItem()->type()))
                    {
                        bReleaseIsNeed = false;
                        if (movingItem->isSelected() && isMultipleSelectionEnabled())
                            movingItem->setSelected(false);
                        else
                            if (movingItem->parentItem() && movingItem->parentItem()->isSelected() && isMultipleSelectionEnabled())
                                movingItem->parentItem()->setSelected(false);
                            else
                            {
                                if (movingItem->isSelected())
                                    bReleaseIsNeed = true;

                                WBGraphicsTextItem* textItem = dynamic_cast<WBGraphicsTextItem*>(movingItem);
                                WBGraphicsMediaItem* movieItem = dynamic_cast<WBGraphicsMediaItem*>(movingItem);
                                if(textItem)
                                    textItem->setSelected(true);
                                else if(movieItem)
                                    movieItem->setSelected(true);
                                else
                                    movingItem->setSelected(true);
                            }

                    }
                }
            }
            else
                bReleaseIsNeed = true;

        if (bReleaseIsNeed)
        {
            QGraphicsView::mouseReleaseEvent (event);
        }
    }
    else if (currentTool == WBStylusTool::Text)
    {
        bool bReleaseIsNeed = true;
        if (movingItem != determineItemToPress(scene()->itemAt(this->mapToScene(event->localPos().toPoint()), QTransform())))
        {
            movingItem = NULL;
            bReleaseIsNeed = false;
        }

        WBGraphicsItem *graphicsItem = dynamic_cast<WBGraphicsItem*>(movingItem);
        if (graphicsItem)
            graphicsItem->Delegate()->commitUndoStep();

        if (mWidgetMoved)
        {
            mWidgetMoved = false;
            movingItem = NULL;
            if (scene () && mRubberBand && mIsCreatingTextZone) {
                QRect rubberRect = mRubberBand->geometry ();

                WBGraphicsTextItem* textItem = scene()->addTextHtml ("", mapToScene (rubberRect.topLeft ()));
                event->accept ();

                WBDrawingController::drawingController ()->setStylusTool (WBStylusTool::Selector);

                textItem->setTextInteractionFlags(Qt::TextEditorInteraction);
                textItem->setSelected (true);
                textItem->setTextWidth(0);
                textItem->setFocus();
            }
        }
        else if (movingItem && (!isCppTool(movingItem) || WBGraphicsCurtainItem::Type == movingItem->type()))
        {
            if (suspendedMousePressEvent)
            {
                QGraphicsView::mousePressEvent(suspendedMousePressEvent);     // suspendedMousePressEvent is deleted by old Qt event loop
                movingItem = NULL;
                delete suspendedMousePressEvent;
                suspendedMousePressEvent = NULL;
                bReleaseIsNeed = true;
            }
            else{
                if (isWBItem(movingItem) &&
                        DelegateButton::Type != movingItem->type() &&
                        QGraphicsSvgItem::Type !=  movingItem->type() &&
                        WBGraphicsDelegateFrame::Type !=  movingItem->type() &&
                        WBGraphicsCache::Type != movingItem->type() &&
                        //QGraphicsView::Type != movingItem->type() && // for W3C widgets as Tools.
                        !(!isMultipleSelectionEnabled() && movingItem->parentItem() && WBGraphicsWidgetItem::Type == movingItem->type() && WBGraphicsGroupContainerItem::Type == movingItem->parentItem()->type()))
                {
                    bReleaseIsNeed = false;
                    if (movingItem->isSelected() && isMultipleSelectionEnabled())
                        movingItem->setSelected(false);
                    else
                        if (movingItem->parentItem() && movingItem->parentItem()->isSelected() && isMultipleSelectionEnabled())
                            movingItem->parentItem()->setSelected(false);
                        else
                        {
                            if (movingItem->isSelected())
                                bReleaseIsNeed = true;

                            movingItem->setSelected(true);
                        }

                }
            }
        }
        else
            bReleaseIsNeed = true;

        if (bReleaseIsNeed)
        {
            QGraphicsView::mouseReleaseEvent (event);
        }
    }
    else if (currentTool == WBStylusTool::Play) {
        if (bIsDesktop) {
            event->ignore();
            return;
        }

        if (mWidgetMoved) {
            movingItem->setSelected(false);
            movingItem = NULL;
            mWidgetMoved = false;
        }
        else {
            if (suspendedMousePressEvent) {
                QGraphicsView::mousePressEvent(suspendedMousePressEvent);     // suspendedMousePressEvent is deleted by old Qt event loop
                movingItem = NULL;
                delete suspendedMousePressEvent;
                suspendedMousePressEvent = NULL;
            }
        }
        QGraphicsView::mouseReleaseEvent (event);
    }
    else if (currentTool == WBStylusTool::Capture)
    {

        if (scene () && mRubberBand && mIsCreatingSceneGrabZone && mRubberBand->geometry ().width () > 16)
        {
            QRect rect = mRubberBand->geometry ();
            QPointF sceneTopLeft = mapToScene (rect.topLeft ());
            QPointF sceneBottomRight = mapToScene (rect.bottomRight ());
            QRectF sceneRect (sceneTopLeft, sceneBottomRight);

            mController->grabScene (sceneRect);

            event->accept ();
        }
        else
        {
            QGraphicsView::mouseReleaseEvent (event);
        }

        mIsCreatingSceneGrabZone = false;
    }
    else
    {
        if (mPendingStylusReleaseEvent || mMouseButtonIsPressed)
        {
            event->accept ();
        }
    }


    if (mUBRubberBand) {
        mUBRubberBand->hide();
        delete mUBRubberBand;
        mUBRubberBand = NULL;
    }

    if (mRubberBand) {
        mRubberBand->hide();
        delete mRubberBand;
        mRubberBand = NULL;
    }

    mMouseButtonIsPressed = false;
    mPendingStylusReleaseEvent = false;
    mTabletStylusIsPressed = false;
    movingItem = NULL;

    mLongPressTimer.stop();
    emit mouseReleased();
}

void WBBoardView::forcedTabletRelease ()
{
    if (mMouseButtonIsPressed || mTabletStylusIsPressed || mPendingStylusReleaseEvent)
    {
        qWarning () << "dirty mouse/tablet state:";
        qWarning () << "mMouseButtonIsPressed =" << mMouseButtonIsPressed;
        qWarning () << "mTabletStylusIsPressed = " << mTabletStylusIsPressed;
        qWarning () << "mPendingStylusReleaseEvent" << mPendingStylusReleaseEvent;
        qWarning () << "forcing device release";

        scene ()->inputDeviceRelease ();

        mMouseButtonIsPressed = false;
        mTabletStylusIsPressed = false;
        mPendingStylusReleaseEvent = false;
    }
}

void WBBoardView::mouseDoubleClickEvent (QMouseEvent *event)
{
    // We don't want a double click, we want two clicks
    mousePressEvent (event);
}

void WBBoardView::wheelEvent (QWheelEvent *wheelEvent)
{
    QList<QGraphicsItem *> selItemsList = scene()->selectedItems();
    // if NO have selected items, than no need process mouse wheel. just exist
    if( selItemsList.count() > 0 )
    {
        // only one selected item possible, so we will work with first item only
        QGraphicsItem * selItem = selItemsList[0];

        // get items list under mouse cursor
        QPointF scenePos = mapToScene(wheelEvent->pos());
        QList<QGraphicsItem *> itemsList = scene()->items(scenePos);

        bool isSelectedAndMouseHower = itemsList.contains(selItem);
        if(isSelectedAndMouseHower)
        {
            QGraphicsView::wheelEvent(wheelEvent);
            wheelEvent->accept();
        }

    }

}

void WBBoardView::leaveEvent (QEvent * event)
{
    if (scene ())
        scene ()->leaveEvent (event);

    mJustSelectedItems.clear();

    QGraphicsView::leaveEvent (event);
}

void WBBoardView::drawItems (QPainter *painter, int numItems, QGraphicsItem* items[], const QStyleOptionGraphicsItem options[])
{
    if (!mFilterZIndex)
        QGraphicsView::drawItems (painter, numItems, items, options);
    else
    {
        int count = 0;

        QGraphicsItem** itemsFiltered = new QGraphicsItem*[numItems];
        QStyleOptionGraphicsItem *optionsFiltered = new QStyleOptionGraphicsItem[numItems];

        for (int i = 0; i < numItems; i++)
        {
            if (shouldDisplayItem (items[i]))
            {
                itemsFiltered[count] = items[i];
                optionsFiltered[count] = options[i];
                count++;
            }
        }

        QGraphicsView::drawItems (painter, count, itemsFiltered, optionsFiltered);

        delete[] optionsFiltered;
        delete[] itemsFiltered;
    }
}


void WBBoardView::dragMoveEvent(QDragMoveEvent *event)
{
    QGraphicsView::dragMoveEvent(event);
    event->acceptProposedAction();
}

void WBBoardView::dropEvent (QDropEvent *event)
{
    QGraphicsItem *onItem = itemAt(event->pos().x(),event->pos().y());
    if (onItem && onItem->type() == WBGraphicsWidgetItem::Type) {
        QGraphicsView::dropEvent(event);
    }
    else {
        if (!event->source()
                || qobject_cast<WBThumbnailWidget *>(event->source())
                || qobject_cast<QWebEngineView*>(event->source())
                || qobject_cast<QListView *>(event->source())) {
            mController->processMimeData (event->mimeData (), mapToScene (event->pos ()));
            event->acceptProposedAction();
        }
    }
    //prevent features in UBFeaturesWidget deletion from the model when event is processing inside
    //Qt base classes
    if (event->dropAction() == Qt::MoveAction) {
        event->setDropAction(Qt::CopyAction);
    }
}

void WBBoardView::resizeEvent (QResizeEvent * event)
{
    const qreal maxWidth = width () * 10;
    const qreal maxHeight = height () * 10;

    setSceneRect (-(maxWidth / 2), -(maxHeight / 2), maxWidth, maxHeight);
    centerOn (0, 0);

    emit resized (event);
}

void WBBoardView::drawBackground (QPainter *painter, const QRectF &rect)
{
    if (testAttribute (Qt::WA_TranslucentBackground))
    {
        QGraphicsView::drawBackground (painter, rect);
        return;
    }

    bool darkBackground = scene () && scene ()->isDarkBackground ();

    if (darkBackground)
    {
        painter->fillRect (rect, QBrush (QColor (Qt::black)));
    }
    else
    {
        painter->fillRect (rect, QBrush (QColor (Qt::white)));
    }

    if (transform ().m11 () > 0.5)
    {
        QColor bgCrossColor;

        if (darkBackground)
            bgCrossColor = QColor(WBSettings::settings()->boardCrossColorDarkBackground->get().toString());
        else
            bgCrossColor = QColor(WBSettings::settings()->boardCrossColorLightBackground->get().toString());

        if (transform ().m11 () < 0.7)
        {
            int alpha = 255 * transform ().m11 () / 2;
            bgCrossColor.setAlpha (alpha); // fade the crossing on small zooms
        }

        qreal gridSize = scene()->backgroundGridSize();

        painter->setPen (bgCrossColor);

        if (scene () && scene ()->pageBackground() == WBPageBackground::crossed)
        {
            qreal firstY = ((int) (rect.y () / gridSize)) * gridSize;

            for (qreal yPos = firstY; yPos < rect.y () + rect.height (); yPos += gridSize)
            {
                painter->drawLine (rect.x (), yPos, rect.x () + rect.width (), yPos);
            }

            qreal firstX = ((int) (rect.x () / gridSize)) * gridSize;

            for (qreal xPos = firstX; xPos < rect.x () + rect.width (); xPos += gridSize)
            {
                painter->drawLine (xPos, rect.y (), xPos, rect.y () + rect.height ());
            }
        }

        if (scene() && scene()->pageBackground() == WBPageBackground::ruled)
        {
            qreal firstY = ((int) (rect.y () / gridSize)) * gridSize;

            for (qreal yPos = firstY; yPos < rect.y () + rect.height (); yPos += gridSize)
            {
                painter->drawLine (rect.x (), yPos, rect.x () + rect.width (), yPos);
            }
        }
    }

    if (!mFilterZIndex && scene ())
    {
        QSize pageNominalSize = scene ()->nominalSize ();

        if (pageNominalSize.isValid ())
        {
            qreal penWidth = 8.0 / transform ().m11 ();

            QRectF pageRect (pageNominalSize.width () / -2, pageNominalSize.height () / -2
                             , pageNominalSize.width (), pageNominalSize.height ());

            pageRect.adjust (-penWidth / 2, -penWidth / 2, penWidth / 2, penWidth / 2);

            QColor docSizeColor;

            if (darkBackground)
                docSizeColor = WBSettings::documentSizeMarkColorDarkBackground;
            else
                docSizeColor = WBSettings::documentSizeMarkColorLightBackground;

            QPen pen (docSizeColor);
            pen.setWidth (penWidth);
            painter->setPen (pen);
            painter->drawRect (pageRect);
        }
    }
}

void WBBoardView::settingChanged (QVariant newValue)
{
    Q_UNUSED (newValue);

    mPenPressureSensitive = WBSettings::settings ()->boardPenPressureSensitive->get ().toBool ();
    mMarkerPressureSensitive = WBSettings::settings ()->boardMarkerPressureSensitive->get ().toBool ();
    mUseHighResTabletEvent = WBSettings::settings ()->boardUseHighResTabletEvent->get ().toBool ();
}

void WBBoardView::virtualKeyboardActivated(bool b)
{
    WBPlatformUtils::setWindowNonActivableFlag(this, b);
    mVirtualKeyboardActive = b;
    setInteractive(!b);
}


// Apple remote desktop sends funny events when the transmission is bad

bool WBBoardView::isAbsurdPoint(QPoint point)
{
    QDesktopWidget *desktop = qApp->desktop ();
    bool isValidPoint = false;

    for (int i = 0; i < desktop->numScreens (); i++)
    {
        QRect screenRect = desktop->screenGeometry (i);
        isValidPoint = isValidPoint || screenRect.contains (mapToGlobal(point));
    }

    return !isValidPoint;
}

void WBBoardView::focusOutEvent (QFocusEvent * event)
{
    Q_UNUSED (event);
}

void WBBoardView::setToolCursor (int tool)
{
    QWidget *controlViewport = viewport ();
    switch (tool)
    {
    case WBStylusTool::Pen:
        controlViewport->setCursor (WBResources::resources ()->penCursor);
        break;
    case WBStylusTool::Eraser:
        controlViewport->setCursor (WBResources::resources ()->eraserCursor);
        break;
    case WBStylusTool::Marker:
        controlViewport->setCursor (WBResources::resources ()->markerCursor);
        break;
    case WBStylusTool::Pointer:
        controlViewport->setCursor (WBResources::resources ()->pointerCursor);
        break;
    case WBStylusTool::Hand:
        controlViewport->setCursor (WBResources::resources ()->handCursor);
        break;
    case WBStylusTool::ZoomIn:
        controlViewport->setCursor (WBResources::resources ()->zoomInCursor);
        break;
    case WBStylusTool::ZoomOut:
        controlViewport->setCursor (WBResources::resources ()->zoomOutCursor);
        break;
    case WBStylusTool::Selector:
        controlViewport->setCursor (WBResources::resources ()->arrowCursor);
        break;
    case WBStylusTool::Play:
        controlViewport->setCursor (WBResources::resources ()->playCursor);
        break;
    case WBStylusTool::Line:
        controlViewport->setCursor (WBResources::resources ()->penCursor);
        break;
    case WBStylusTool::Text:
        controlViewport->setCursor (WBResources::resources ()->textCursor);
        break;
    case WBStylusTool::Capture:
        controlViewport->setCursor (WBResources::resources ()->penCursor);
        break;
    default:
        Q_ASSERT (false);
        //failsafe
        controlViewport->setCursor (WBResources::resources ()->penCursor);
    }
}


bool WBBoardView::hasSelectedParents(QGraphicsItem * item)
{
    if (item->isSelected())
        return true;
    if (item->parentItem()==NULL)
        return false;
    return hasSelectedParents(item->parentItem());
}
