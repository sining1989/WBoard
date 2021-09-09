#include <QDesktopWidget>

#include "WBDesktopAnnotationController.h"

#include "frameworks/WBPlatformUtils.h"

#include "core/WBApplication.h"
#include "core/WBApplicationController.h"
#include "core/WBDisplayManager.h"
#include "core/WBSettings.h"

#include "web/WBWebController.h"

#include "gui/WBMainWindow.h"

#include "board/WBBoardView.h"
#include "board/WBDrawingController.h"
#include "board/WBBoardController.h"
#include "board/WBBoardPaletteManager.h"

#include "domain/WBGraphicsScene.h"
#include "domain/WBGraphicsPolygonItem.h"

#include "WBCustomCaptureWindow.h"
#include "WBWindowCapture.h"
#include "WBDesktopPalette.h"
#include "WBDesktopPropertyPalette.h"

#include "gui/WBKeyboardPalette.h"
#include "gui/WBResources.h"

#include "core/memcheck.h"

WBDesktopAnnotationController::WBDesktopAnnotationController(QObject *parent, WBRightPalette* rightPalette)
        : QObject(parent)
        , mTransparentDrawingView(0)
        , mTransparentDrawingScene(0)
        , mDesktopPalette(NULL)
        , mDesktopPenPalette(NULL)
        , mDesktopMarkerPalette(NULL)
        , mDesktopEraserPalette(NULL)
        , mRightPalette(rightPalette)
        , mWindowPositionInitialized(false)
        , mIsFullyTransparent(false)
        , mDesktopToolsPalettePositioned(false)
        , mPendingPenButtonPressed(false)
        , mPendingMarkerButtonPressed(false)
        , mPendingEraserButtonPressed(false)
        , mbArrowClicked(false)
        , mBoardStylusTool(WBDrawingController::drawingController()->stylusTool())
        , mDesktopStylusTool(WBDrawingController::drawingController()->stylusTool())
{

    mTransparentDrawingView = new WBBoardView(WBApplication::boardController, static_cast<QWidget*>(0), false, true); // deleted in WBDesktopAnnotationController::destructor
    mTransparentDrawingView->setAttribute(Qt::WA_TranslucentBackground, true);
#ifdef Q_OS_OSX
    mTransparentDrawingView->setAttribute(Qt::WA_MacNoShadow, true);
#endif
    mTransparentDrawingView->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Window | Qt::NoDropShadowWindowHint);
    mTransparentDrawingView->setCacheMode(QGraphicsView::CacheNone);
    mTransparentDrawingView->resize(WBApplication::desktop()->width(), WBApplication::desktop()->height());

    mTransparentDrawingView->setMouseTracking(true);

    mTransparentDrawingView->setAcceptDrops(true);

    QString backgroundStyle = "QWidget {background-color: rgba(127, 127, 127, 0)}";
    mTransparentDrawingView->setStyleSheet(backgroundStyle);

    mTransparentDrawingScene = new WBGraphicsScene(0, false);
    updateColors();

    mTransparentDrawingView->setScene(mTransparentDrawingScene);
    mTransparentDrawingScene->setDrawingMode(true);

    mDesktopPalette = new WBDesktopPalette(mTransparentDrawingView, rightPalette); 
    // This was not fix, parent reverted
    // FIX #633: The palette must be 'floating' in order to stay on top of the library palette

    if (WBPlatformUtils::hasVirtualKeyboard())
    {
        connect( WBApplication::boardController->paletteManager()->mKeyboardPalette, SIGNAL(keyboardActivated(bool)), 
                 mTransparentDrawingView, SLOT(virtualKeyboardActivated(bool)));

#ifdef Q_OS_LINUX
        connect(WBApplication::boardController->paletteManager()->mKeyboardPalette, SIGNAL(moved(QPoint)), this, SLOT(refreshMask()));
        connect(WBApplication::mainWindow->actionVirtualKeyboard, SIGNAL(triggered(bool)), this, SLOT(refreshMask()));
        connect(mDesktopPalette,SIGNAL(refreshMask()), this, SLOT(refreshMask()));
#endif
    }

    connect(mDesktopPalette, SIGNAL(uniboardClick()), this, SLOT(goToUniboard()));
    connect(mDesktopPalette, SIGNAL(customClick()), this, SLOT(customCapture()));
    connect(mDesktopPalette, SIGNAL(windowClick()), this, SLOT(windowCapture()));
    connect(mDesktopPalette, SIGNAL(screenClick()), this, SLOT(screenCapture()));
    connect(WBApplication::mainWindow->actionPointer, SIGNAL(triggered()), this, SLOT(onToolClicked()));
    connect(WBApplication::mainWindow->actionSelector, SIGNAL(triggered()), this, SLOT(onToolClicked()));
    connect(mDesktopPalette, SIGNAL(maximized()), this, SLOT(onDesktopPaletteMaximized()));
    connect(mDesktopPalette, SIGNAL(minimizeStart(eMinimizedLocation)), this, SLOT(onDesktopPaletteMinimize()));
    connect(mDesktopPalette, SIGNAL(mouseEntered()), mTransparentDrawingScene, SLOT(hideTool()));
    connect(mRightPalette, SIGNAL(mouseEntered()), mTransparentDrawingScene, SLOT(hideTool()));

    connect(mTransparentDrawingView, SIGNAL(resized(QResizeEvent*)), this, SLOT(onTransparentWidgetResized()));


    connect(WBDrawingController::drawingController(), SIGNAL(stylusToolChanged(int)), this, SLOT(stylusToolChanged(int)));

    // Add the desktop associated palettes
    mDesktopPenPalette = new WBDesktopPenPalette(mTransparentDrawingView, rightPalette); 

    connect(mDesktopPalette, SIGNAL(maximized()), mDesktopPenPalette, SLOT(onParentMaximized()));
    connect(mDesktopPalette, SIGNAL(minimizeStart(eMinimizedLocation)), mDesktopPenPalette, SLOT(onParentMinimized()));

    mDesktopMarkerPalette = new WBDesktopMarkerPalette(mTransparentDrawingView, rightPalette);
    mDesktopEraserPalette = new WBDesktopEraserPalette(mTransparentDrawingView, rightPalette);

    mDesktopPalette->setBackgroundBrush(WBSettings::settings()->opaquePaletteColor);
    mDesktopPenPalette->setBackgroundBrush(WBSettings::settings()->opaquePaletteColor);
    mDesktopMarkerPalette->setBackgroundBrush(WBSettings::settings()->opaquePaletteColor);
    mDesktopEraserPalette->setBackgroundBrush(WBSettings::settings()->opaquePaletteColor);


    // Hack : the size of the property palettes is computed the first time the palette is visible
    //        In order to prevent palette overlap on if the desktop palette is on the right of the
    //        screen, a setVisible(true) followed by a setVisible(false) is done.
    mDesktopPenPalette->setVisible(true);
    mDesktopMarkerPalette->setVisible(true);
    mDesktopEraserPalette->setVisible(true);
    mDesktopPenPalette->setVisible(false);
    mDesktopMarkerPalette->setVisible(false);
    mDesktopEraserPalette->setVisible(false);

    connect(WBApplication::mainWindow->actionEraseDesktopAnnotations, SIGNAL(triggered()), this, SLOT(eraseDesktopAnnotations()));
    connect(WBApplication::boardController, SIGNAL(backgroundChanged()), this, SLOT(updateColors()));
    connect(WBApplication::boardController, SIGNAL(activeSceneChanged()), this, SLOT(updateColors()));
    connect(&mHoldTimerPen, SIGNAL(timeout()), this, SLOT(penActionReleased()));
    connect(&mHoldTimerMarker, SIGNAL(timeout()), this, SLOT(markerActionReleased()));
    connect(&mHoldTimerEraser, SIGNAL(timeout()), this, SLOT(eraserActionReleased()));

#ifdef Q_OS_LINUX
    connect(mDesktopPalette, SIGNAL(moving()), this, SLOT(refreshMask()));
    connect(WBApplication::boardController->paletteManager()->rightPalette(), SIGNAL(resized()), this, SLOT(refreshMask()));
    connect(WBApplication::boardController->paletteManager()->addItemPalette(), SIGNAL(closed()), this, SLOT(refreshMask()));
#endif
    onDesktopPaletteMaximized();

    // FIX #633: Ensure that these palettes stay on top of the other elements
    //mDesktopEraserPalette->raise();
    //mDesktopMarkerPalette->raise();
    //mDesktopPenPalette->raise();
}

WBDesktopAnnotationController::~WBDesktopAnnotationController()
{
    delete mTransparentDrawingScene;
    delete mTransparentDrawingView;
}

void WBDesktopAnnotationController::updateColors(){
    if(WBApplication::boardController->activeScene()->isDarkBackground()){
        mTransparentDrawingScene->setBackground(true, WBPageBackground::plain);
    }else{
        mTransparentDrawingScene->setBackground(false, WBPageBackground::plain);
    }
}

WBDesktopPalette* WBDesktopAnnotationController::desktopPalette()
{
    return mDesktopPalette;
}

QPainterPath WBDesktopAnnotationController::desktopPalettePath() const
{
    QPainterPath result;
    if (mDesktopPalette && mDesktopPalette->isVisible()) {
        result.addRect(mDesktopPalette->geometry());
    }
    if (mDesktopPenPalette && mDesktopPenPalette->isVisible()) {
        result.addRect(mDesktopPenPalette->geometry());
    }
    if (mDesktopMarkerPalette && mDesktopMarkerPalette->isVisible()) {
        result.addRect(mDesktopMarkerPalette->geometry());
    }
    if (mDesktopEraserPalette && mDesktopEraserPalette->isVisible()) {
        result.addRect(mDesktopEraserPalette->geometry());
    }

    return result;
}

/**
 * \brief Toggle the visibility of the pen associated palette
 * @param checked as the visibility state
 */
void WBDesktopAnnotationController::desktopPenActionToggled(bool checked)
{
    setAssociatedPalettePosition(mDesktopPenPalette, "actionPen");
    mDesktopPenPalette->setVisible(checked);
    mDesktopMarkerPalette->setVisible(false);
    mDesktopEraserPalette->setVisible(false);
}

/**
 * \brief Toggle the visibility of the marker associated palette
 * @param checked as the visibility state
 */
void WBDesktopAnnotationController::desktopMarkerActionToggled(bool checked)
{
    setAssociatedPalettePosition(mDesktopMarkerPalette, "actionMarker");
    mDesktopMarkerPalette->setVisible(checked);
    mDesktopPenPalette->setVisible(false);
    mDesktopEraserPalette->setVisible(false);
}

/**
 * \brief Toggle the visibility of the eraser associated palette
 * @param checked as the visibility state
 */
void WBDesktopAnnotationController::desktopEraserActionToggled(bool checked)
{
    setAssociatedPalettePosition(mDesktopEraserPalette, "actionEraser");
    mDesktopEraserPalette->setVisible(checked);
    mDesktopPenPalette->setVisible(false);
    mDesktopMarkerPalette->setVisible(false);
}

/**
 * \brief Set the location of the properties palette
 * @param palette as the palette
 * @param actionName as the name of the related action
 */
void WBDesktopAnnotationController::setAssociatedPalettePosition(WBActionPalette *palette, const QString &actionName)
{
    QPoint desktopPalettePos = mDesktopPalette->geometry().topLeft();
    QList<QAction*> actions = mDesktopPalette->actions();
    int yPen = 0;

    foreach(QAction* act, actions)
    {
        if(act->objectName() == actionName)
        {
            int iAction = actions.indexOf(act);
            yPen = iAction * (mDesktopPalette->buttonSize().height() + 2 * mDesktopPalette->border() +6); // This is the mysterious value (6)
            break;
        }
    }

    // First determine if the palette must be shown on the left or on the right
    if(desktopPalettePos.x() <= (mTransparentDrawingView->width() - (palette->width() + mDesktopPalette->width() + mRightPalette->width() + 20))) // we take a small margin of 20 pixels
    {
        // Display it on the right
        desktopPalettePos += QPoint(mDesktopPalette->width(), yPen);
    }
    else
    {
        // Display it on the left
        desktopPalettePos += QPoint(0 - palette->width(), yPen);
    }

    palette->setCustomPosition(true);
    palette->move(desktopPalettePos);
}

void WBDesktopAnnotationController::eraseDesktopAnnotations()
{
    if (mTransparentDrawingScene)
    {
        mTransparentDrawingScene->clearContent(WBGraphicsScene::clearAnnotations);
    }
}


WBBoardView* WBDesktopAnnotationController::drawingView()
{
    return mTransparentDrawingView;
}


void WBDesktopAnnotationController::showWindow()
{
    mDesktopPalette->setDisplaySelectButtonVisible(true);

    connect(WBApplication::applicationController, SIGNAL(desktopMode(bool))
            , mDesktopPalette, SLOT(setDisplaySelectButtonVisible(bool)));

    mDesktopPalette->show();

    bool showDisplay = WBSettings::settings()->webShowPageImmediatelyOnMirroredScreen->get().toBool();

    mDesktopPalette->showHideClick(showDisplay);
    mDesktopPalette->updateShowHideState(showDisplay);

    if (!mWindowPositionInitialized)
    {
        QRect desktopRect = QApplication::desktop()->screenGeometry(mDesktopPalette->pos());

        mDesktopPalette->move(5, desktopRect.top() + 150);

        mWindowPositionInitialized = true;
        mDesktopPalette->maximizeMe();
    }

    updateBackground();

    mBoardStylusTool = WBDrawingController::drawingController()->stylusTool();

    WBDrawingController::drawingController()->setStylusTool(mDesktopStylusTool);

#ifndef Q_OS_LINUX
    WBPlatformUtils::showFullScreen(mTransparentDrawingView);
#else
    // this is necessary to avoid hiding the panels on Unity and Cinnamon
    // if finer control is necessary, use qgetenv("XDG_CURRENT_DESKTOP")
    mTransparentDrawingView->show();
#endif
    WBPlatformUtils::setDesktopMode(true);

    mDesktopPalette->appear();

#ifdef Q_OS_LINUX
    updateMask(true);
#endif
}


void WBDesktopAnnotationController::close()
{
    if (mTransparentDrawingView)
        mTransparentDrawingView->hide();

    mDesktopPalette->hide();
}


void WBDesktopAnnotationController::stylusToolChanged(int tool)
{
    Q_UNUSED(tool);

    updateBackground();
}


void WBDesktopAnnotationController::updateBackground()
{
    QBrush newBrush;

    if (mIsFullyTransparent
            || WBDrawingController::drawingController()->stylusTool() == WBStylusTool::Selector)
    {
        newBrush = QBrush(Qt::transparent);
#ifdef Q_OS_LINUX
        updateMask(true);
#endif
    }
    else
    {
#if defined(Q_OS_OSX)
        newBrush = QBrush(QColor(127, 127, 127, 15));
#else
        newBrush = QBrush(QColor(127, 127, 127, 1));
#endif
#ifdef Q_OS_LINUX
        updateMask(false);
#endif
    }

    if (mTransparentDrawingScene && mTransparentDrawingScene->backgroundBrush() != newBrush)
        mTransparentDrawingScene->setBackgroundBrush(newBrush);
}


void WBDesktopAnnotationController::hideWindow()
{
    if (mTransparentDrawingView)
        mTransparentDrawingView->hide();

    mDesktopPalette->hide();

    mDesktopStylusTool = WBDrawingController::drawingController()->stylusTool();
    WBDrawingController::drawingController()->setStylusTool(mBoardStylusTool);
}


void WBDesktopAnnotationController::goToUniboard()
{
    WBApplication::applicationController->showBoard();
}

void WBDesktopAnnotationController::customCapture()
{
    onToolClicked();
    mIsFullyTransparent = true;
    updateBackground();

    mDesktopPalette->disappearForCapture();
    WBCustomCaptureWindow customCaptureWindow(mDesktopPalette);
    // need to show the window before execute it to avoid some glitch on windows.

#ifndef Q_OS_WIN // Working only without this call on win32 desktop mode
    WBPlatformUtils::showFullScreen(&customCaptureWindow);
#endif

    if (customCaptureWindow.execute(getScreenPixmap()) == QDialog::Accepted)
    {
        QPixmap selectedPixmap = customCaptureWindow.getSelectedPixmap();
        emit imageCaptured(selectedPixmap, false);
    }

    mDesktopPalette->appear();

    mIsFullyTransparent = false;
    updateBackground();
}


void WBDesktopAnnotationController::windowCapture()
{
    onToolClicked();
    mIsFullyTransparent = true;
    updateBackground();

    mDesktopPalette->disappearForCapture();

    WBWindowCapture util(this);

    if (util.execute() == QDialog::Accepted)
    {
        QPixmap windowPixmap = util.getCapturedWindow();

        // on Mac OS X we can only know that user cancel the operatiion by checking is the image is null
        // because the screencapture utility always return code 0 event if user cancel the application
        if (!windowPixmap.isNull())
        {
            emit imageCaptured(windowPixmap, false);
        }
    }

    mDesktopPalette->appear();

    mIsFullyTransparent = false;

    updateBackground();
}


void WBDesktopAnnotationController::screenCapture()
{
    onToolClicked();
    mIsFullyTransparent = true;
    updateBackground();

    mDesktopPalette->disappearForCapture();

    QPixmap originalPixmap = getScreenPixmap();

    mDesktopPalette->appear();

    emit imageCaptured(originalPixmap, false);

    mIsFullyTransparent = false;

    updateBackground();
}


QPixmap WBDesktopAnnotationController::getScreenPixmap()
{
    QDesktopWidget *desktop = QApplication::desktop();
    QScreen * screen = WBApplication::controlScreen();

    QRect rect = desktop->screenGeometry(QCursor::pos());

    return screen->grabWindow(desktop->effectiveWinId(),
                              rect.x(), rect.y(), rect.width(), rect.height());
}


void WBDesktopAnnotationController::updateShowHideState(bool pEnabled)
{
    mDesktopPalette->updateShowHideState(pEnabled);
}


void WBDesktopAnnotationController::screenLayoutChanged()
{
    if (WBApplication::applicationController &&
            WBApplication::applicationController->displayManager() &&
            WBApplication::applicationController->displayManager()->hasDisplay())
    {
        mDesktopPalette->setShowHideButtonVisible(true);
    }
    else
    {
        mDesktopPalette->setShowHideButtonVisible(false);
    }
}

/**
 * \brief Handles the pen action pressed event
 */
void WBDesktopAnnotationController::penActionPressed()
{
    mbArrowClicked = false;
    mDesktopMarkerPalette->hide();
    mDesktopEraserPalette->hide();
    WBDrawingController::drawingController()->setStylusTool(WBStylusTool::Pen);
    mPenHoldTimer = QTime::currentTime();
    mPendingPenButtonPressed = true;

    // Check if the mouse cursor is on the little arrow
    QPoint cursorPos = QCursor::pos();
    QPoint palettePos = mDesktopPalette->pos();
    QPoint buttonPos = mDesktopPalette->buttonPos(WBApplication::mainWindow->actionPen);

    int iX = cursorPos.x() - (palettePos.x() + buttonPos.x());    // x position of the cursor in the palette
    int iY = cursorPos.y() - (palettePos.y() + buttonPos.y());    // y position of the cursor in the palette

    if(iX >= 30 && iX <= 40 && iY >= 30 && iY <= 40)
    {
        mbArrowClicked = true;
        penActionReleased();
    }
    else
    {
        mHoldTimerPen.start(PROPERTY_PALETTE_TIMER);
    }
}

/**
 * \brief Handles the pen action released event
 */
void WBDesktopAnnotationController::penActionReleased()
{
    mHoldTimerPen.stop();
    if(mPendingPenButtonPressed)
    {
        if(mbArrowClicked || mPenHoldTimer.msecsTo(QTime::currentTime()) > PROPERTY_PALETTE_TIMER - 100)
        {
            togglePropertyPalette(mDesktopPenPalette);
        }
        else
        {
            WBApplication::mainWindow->actionPen->trigger();
        }
        mPendingPenButtonPressed = false;
    }
    WBApplication::mainWindow->actionPen->setChecked(true);

    switchCursor(WBStylusTool::Pen);
}

/**
 * \brief Handles the eraser action pressed event
 */
void WBDesktopAnnotationController::eraserActionPressed()
{
    mbArrowClicked = false;
    mDesktopPenPalette->hide();
    mDesktopMarkerPalette->hide();
    WBDrawingController::drawingController()->setStylusTool(WBStylusTool::Eraser);
    mEraserHoldTimer = QTime::currentTime();
    mPendingEraserButtonPressed = true;

    // Check if the mouse cursor is on the little arrow
    QPoint cursorPos = QCursor::pos();
    QPoint palettePos = mDesktopPalette->pos();
    QPoint buttonPos = mDesktopPalette->buttonPos(WBApplication::mainWindow->actionEraser);

    int iX = cursorPos.x() - (palettePos.x() + buttonPos.x());    // x position of the cursor in the palette
    int iY = cursorPos.y() - (palettePos.y() + buttonPos.y());    // y position of the cursor in the palette

    if(iX >= 30 && iX <= 40 && iY >= 30 && iY <= 40)
    {
        mbArrowClicked = true;
        eraserActionReleased();
    }
    else
    {
        mHoldTimerEraser.start(PROPERTY_PALETTE_TIMER);
    }
}

/**
 * \brief Handles the eraser action released event
 */
void WBDesktopAnnotationController::eraserActionReleased()
{
    mHoldTimerEraser.stop();
    if(mPendingEraserButtonPressed)
    {
        if(mbArrowClicked || mEraserHoldTimer.msecsTo(QTime::currentTime()) > PROPERTY_PALETTE_TIMER - 100)
        {
            togglePropertyPalette(mDesktopEraserPalette);
        }
        else
        {
            WBApplication::mainWindow->actionEraser->trigger();
        }
        mPendingEraserButtonPressed = false;
    }
    WBApplication::mainWindow->actionEraser->setChecked(true);

    switchCursor(WBStylusTool::Eraser);
}


/**
 * \brief Handles the marker action pressed event
 */
void WBDesktopAnnotationController::markerActionPressed()
{
    mbArrowClicked = false;
    mDesktopPenPalette->hide();
    mDesktopEraserPalette->hide();
    WBDrawingController::drawingController()->setStylusTool(WBStylusTool::Marker);
    mMarkerHoldTimer = QTime::currentTime();
    mPendingMarkerButtonPressed = true;

    // Check if the mouse cursor is on the little arrow
    QPoint cursorPos = QCursor::pos();
    QPoint palettePos = mDesktopPalette->pos();
    QPoint buttonPos = mDesktopPalette->buttonPos(WBApplication::mainWindow->actionMarker);

    int iX = cursorPos.x() - (palettePos.x() + buttonPos.x());    // x position of the cursor in the palette
    int iY = cursorPos.y() - (palettePos.y() + buttonPos.y());    // y position of the cursor in the palette

    if(iX >= 30 && iX <= 40 && iY >= 30 && iY <= 40)
    {
        mbArrowClicked = true;
        markerActionReleased();
    }
    else
    {
        mHoldTimerMarker.start(PROPERTY_PALETTE_TIMER);
    }
}


/**
 * \brief Handles the marker action released event
 */
void WBDesktopAnnotationController::markerActionReleased()
{
    mHoldTimerMarker.stop();
    if(mPendingMarkerButtonPressed)
    {
        if(mbArrowClicked || mMarkerHoldTimer.msecsTo(QTime::currentTime()) > PROPERTY_PALETTE_TIMER - 100)
        {
            togglePropertyPalette(mDesktopMarkerPalette);
        }
        else
        {
            WBApplication::mainWindow->actionMarker->trigger();
        }
        mPendingMarkerButtonPressed = false;
    }
    WBApplication::mainWindow->actionMarker->setChecked(true);

    switchCursor(WBStylusTool::Marker);
}

void WBDesktopAnnotationController::selectorActionPressed()
{

}

void WBDesktopAnnotationController::selectorActionReleased()
{
    WBApplication::mainWindow->actionSelector->setChecked(true);
    switchCursor(WBStylusTool::Selector);
}


void WBDesktopAnnotationController::pointerActionPressed()
{

}

void WBDesktopAnnotationController::pointerActionReleased()
{
    WBApplication::mainWindow->actionPointer->setChecked(true);
    switchCursor(WBStylusTool::Pointer);
}


/**
 * \brief Toggle the given palette visibility
 * @param palette as the given palette
 */
void WBDesktopAnnotationController::togglePropertyPalette(WBActionPalette *palette)
{
    if(NULL != palette)
    {
        bool bShow = !palette->isVisible();
        if(mDesktopPenPalette == palette)
        {
            desktopPenActionToggled(bShow);
        }
        else if(mDesktopMarkerPalette == palette)
        {
            desktopMarkerActionToggled(bShow);
        }
        else if(mDesktopEraserPalette == palette)
        {
            desktopEraserActionToggled(bShow);
        }
    }
}


void WBDesktopAnnotationController::switchCursor(const int tool)
{
    mTransparentDrawingScene->setToolCursor(tool);
    mTransparentDrawingView->setToolCursor(tool);
}

/**
 * \brief Reconnect the pressed & released signals of the property palettes
 */
void WBDesktopAnnotationController::onDesktopPaletteMaximized()
{
    // Pen
    WBActionPaletteButton* pPenButton = mDesktopPalette->getButtonFromAction(WBApplication::mainWindow->actionPen);
    if(NULL != pPenButton)
    {
        connect(pPenButton, SIGNAL(pressed()), this, SLOT(penActionPressed()));
        connect(pPenButton, SIGNAL(released()), this, SLOT(penActionReleased()));
    }

    // Eraser
    WBActionPaletteButton* pEraserButton = mDesktopPalette->getButtonFromAction(WBApplication::mainWindow->actionEraser);
    if(NULL != pEraserButton)
    {
        connect(pEraserButton, SIGNAL(pressed()), this, SLOT(eraserActionPressed()));
        connect(pEraserButton, SIGNAL(released()), this, SLOT(eraserActionReleased()));
    }

    // Marker
    WBActionPaletteButton* pMarkerButton = mDesktopPalette->getButtonFromAction(WBApplication::mainWindow->actionMarker);
    if(NULL != pMarkerButton)
    {
        connect(pMarkerButton, SIGNAL(pressed()), this, SLOT(markerActionPressed()));//zhusizhi 20210625
        connect(pMarkerButton, SIGNAL(released()), this, SLOT(markerActionReleased()));
    }

    // Pointer
    WBActionPaletteButton* pSelectorButton = mDesktopPalette->getButtonFromAction(WBApplication::mainWindow->actionSelector);
    if(NULL != pSelectorButton)
    {
        connect(pSelectorButton, SIGNAL(pressed()), this, SLOT(selectorActionPressed()));
        connect(pSelectorButton, SIGNAL(released()), this, SLOT(selectorActionReleased()));
    }

    // Pointer
    WBActionPaletteButton* pPointerButton = mDesktopPalette->getButtonFromAction(WBApplication::mainWindow->actionPointer);
    if(NULL != pPointerButton)
    {
        connect(pPointerButton, SIGNAL(pressed()), this, SLOT(pointerActionPressed()));
        connect(pPointerButton, SIGNAL(released()), this, SLOT(pointerActionReleased()));
    }
}

/**
 * \brief Disconnect the pressed & release signals of the property palettes
 * This is done to prevent memory leaks
 */
void WBDesktopAnnotationController::onDesktopPaletteMinimize()
{
    // Pen
    WBActionPaletteButton* pPenButton = mDesktopPalette->getButtonFromAction(WBApplication::mainWindow->actionPen);
    if(NULL != pPenButton)
    {
        disconnect(pPenButton, SIGNAL(pressed()), this, SLOT(penActionPressed()));
        disconnect(pPenButton, SIGNAL(released()), this, SLOT(penActionReleased()));
    }

    // Marker
    WBActionPaletteButton* pMarkerButton = mDesktopPalette->getButtonFromAction(WBApplication::mainWindow->actionMarker);
    if(NULL != pMarkerButton)
    {
        disconnect(pMarkerButton, SIGNAL(pressed()), this, SLOT(markerActionPressed()));
        disconnect(pMarkerButton, SIGNAL(released()), this, SLOT(markerActionReleased()));
    }

    // Eraser
    WBActionPaletteButton* pEraserButton = mDesktopPalette->getButtonFromAction(WBApplication::mainWindow->actionEraser);
    if(NULL != pEraserButton)
    {
        disconnect(pEraserButton, SIGNAL(pressed()), this, SLOT(eraserActionPressed()));
        disconnect(pEraserButton, SIGNAL(released()), this, SLOT(eraserActionReleased()));
    }
}

void WBDesktopAnnotationController::TransparentWidgetResized()
{
    onTransparentWidgetResized();
}

/**
 * \brief Resize the library palette.
 */
void WBDesktopAnnotationController::onTransparentWidgetResized()
{
    int rW = WBApplication::boardController->paletteManager()->rightPalette()->width();
    int lW = WBApplication::boardController->paletteManager()->leftPalette()->width();

    int rH = mTransparentDrawingView->height();

    WBApplication::boardController->paletteManager()->rightPalette()->resize(rW+1, rH);
    WBApplication::boardController->paletteManager()->rightPalette()->resize(rW, rH);

    WBApplication::boardController->paletteManager()->leftPalette()->resize(lW+1, rH);
    WBApplication::boardController->paletteManager()->leftPalette()->resize(lW, rH);
}

void WBDesktopAnnotationController::updateMask(bool bTransparent)
{
    if(bTransparent)
    {
        // Here we have to generate a new mask This method is certainly resource
        // consuming but for the moment this is the only solution that I found.
        mMask = QPixmap(mTransparentDrawingView->width(), mTransparentDrawingView->height());
        mMask.fill(Qt::transparent);

        QPainter p;

        p.begin(&mMask);

        p.setPen(Qt::red);
        p.setBrush(QBrush(Qt::red));

        // Here we draw the widget mask
        if(mDesktopPalette->isVisible())
        {
            p.drawRect(mDesktopPalette->geometry().x(), mDesktopPalette->geometry().y(), mDesktopPalette->width(), mDesktopPalette->height());
        }
        if(WBApplication::boardController->paletteManager()->mKeyboardPalette->isVisible())
        {
            p.drawRect(WBApplication::boardController->paletteManager()->mKeyboardPalette->geometry().x(), WBApplication::boardController->paletteManager()->mKeyboardPalette->geometry().y(),
                       WBApplication::boardController->paletteManager()->mKeyboardPalette->width(), WBApplication::boardController->paletteManager()->mKeyboardPalette->height());
        }

        if(WBApplication::boardController->paletteManager()->leftPalette()->isVisible())
        {
            QRect leftPalette(WBApplication::boardController->paletteManager()->leftPalette()->geometry().x(),
                        WBApplication::boardController->paletteManager()->leftPalette()->geometry().y(),
                        WBApplication::boardController->paletteManager()->leftPalette()->width(),
                        WBApplication::boardController->paletteManager()->leftPalette()->height());

            QRect tabsPalette(WBApplication::boardController->paletteManager()->leftPalette()->getTabPaletteRect());

            p.drawRect(leftPalette);
            p.drawRect(tabsPalette);
        }

        if(WBApplication::boardController->paletteManager()->rightPalette()->isVisible())
        {
            QRect rightPalette(WBApplication::boardController->paletteManager()->rightPalette()->geometry().x(),
                        WBApplication::boardController->paletteManager()->rightPalette()->geometry().y(),
                        WBApplication::boardController->paletteManager()->rightPalette()->width(),
                        WBApplication::boardController->paletteManager()->rightPalette()->height());

            QRect tabsPalette(WBApplication::boardController->paletteManager()->rightPalette()->getTabPaletteRect());

            p.drawRect(rightPalette);
            p.drawRect(tabsPalette);
        }

#ifdef Q_OS_LINUX
        //Rquiered only for compiz wm
        //TODO. Window manager detection screen

        if (WBApplication::boardController->paletteManager()->addItemPalette()->isVisible()) {
            p.drawRect(WBApplication::boardController->paletteManager()->addItemPalette()->geometry());
        }

#endif

        p.end();

        // Then we add the annotations. We create another painter because we need to
        // apply transformations on it for coordinates matching
        QPainter annotationPainter;

        QTransform trans;
        trans.translate(mTransparentDrawingView->width()/2, mTransparentDrawingView->height()/2);

        annotationPainter.begin(&mMask);
        annotationPainter.setPen(Qt::red);
        annotationPainter.setBrush(Qt::red);

        annotationPainter.setTransform(trans);

        QList<QGraphicsItem*> allItems = mTransparentDrawingScene->items();

        for(int i = 0; i < allItems.size(); i++)
        {
            QGraphicsItem* pCrntItem = allItems.at(i);

            if(pCrntItem->isVisible() && pCrntItem->type() == WBGraphicsPolygonItem::Type)
            {
                QPainterPath crntPath = pCrntItem->shape();
                QRectF rect = crntPath.boundingRect();

                annotationPainter.drawRect(rect);
            }
        }

        annotationPainter.end();

        mTransparentDrawingView->setMask(mMask.mask());
    }
    else
    {
        // Remove the mask
        QPixmap noMask(mTransparentDrawingView->width(), mTransparentDrawingView->height());
        mTransparentDrawingView->setMask(noMask.mask());
    }
}

void WBDesktopAnnotationController::refreshMask()
{
    if (mTransparentDrawingScene && mTransparentDrawingView->isVisible()) {
        if(mIsFullyTransparent
                || WBDrawingController::drawingController()->stylusTool() == WBStylusTool::Selector
                //Needed to work correctly when another actions on stylus are checked
                || WBDrawingController::drawingController()->stylusTool() == WBStylusTool::Eraser
                || WBDrawingController::drawingController()->stylusTool() == WBStylusTool::Pointer
                || WBDrawingController::drawingController()->stylusTool() == WBStylusTool::Pen
                || WBDrawingController::drawingController()->stylusTool() == WBStylusTool::Marker)
        {
            updateMask(true);
        }
    }
}

void WBDesktopAnnotationController::onToolClicked()
{
    mDesktopEraserPalette->hide();
    mDesktopMarkerPalette->hide();
    mDesktopPenPalette->hide();
}
