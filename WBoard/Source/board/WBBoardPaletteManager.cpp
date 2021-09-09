#include "WBBoardPaletteManager.h"

#include "frameworks/WBPlatformUtils.h"
#include "frameworks/WBFileSystemUtils.h"

#include "core/WBApplication.h"
#include "core/WBApplicationController.h"
#include "core/WBSettings.h"
#include "core/WBSetting.h"
#include "core/WBDisplayManager.h"

#include "gui/WBMainWindow.h"
#include "gui/WBStylusPalette.h"
#include "gui/WBKeyboardPalette.h"
#include "gui/WBToolWidget.h"
#include "gui/WBZoomPalette.h"
#include "gui/WBWebToolsPalette.h"
#include "gui/WBActionPalette.h"
#include "gui/WBBackgroundPalette.h"
#include "gui/WBFavoriteToolPalette.h"
#include "gui/WBStartupHintsPalette.h"

#include "web/WBWebPage_.h"
#include "web/WBWebController.h"
#include "web/browser/WBBrowserWindow.h"
#include "web/browser/WBTabWidget.h"
#include "web/browser/WBWebView.h"

#include "desktop/WBDesktopAnnotationController.h"


#include "network/WBNetworkAccessManager.h"
//#include "network/WBServerXMLHttpRequest.h"

#include "domain/WBGraphicsScene.h"
#include "domain/WBGraphicsPixmapItem.h"

#include "document/WBDocumentProxy.h"
#include "podcast/WBPodcastController.h"
#include "board/WBDrawingController.h"

#include "tools/WBToolsManager.h"

#include "WBBoardController.h"

#include "document/WBDocumentController.h"

#include "core/WBPersistenceManager.h"
#include "core/memcheck.h"

WBBoardPaletteManager::WBBoardPaletteManager(QWidget* container, WBBoardController* pBoardController)
    : QObject(container)
    , mKeyboardPalette(0)
    , mWebToolsCurrentPalette(0)
    , mContainer(container)
    , mBoardControler(pBoardController)
    , mStylusPalette(0)
    , mZoomPalette(0)
    , mTipPalette(0)
    , mLeftPalette(NULL)
    , mRightPalette(NULL)
    , mBackgroundsPalette(0)
    , mToolsPalette(0)
    , mAddItemPalette(0)
    , mErasePalette(NULL)
    , mPagePalette(NULL)
    , mPendingPageButtonPressed(false)
    , mPendingZoomButtonPressed(false)
    , mPendingPanButtonPressed(false)
    , mPendingEraseButtonPressed(false)
    , mpPageNavigWidget(NULL)
    , mpCachePropWidget(NULL)
    , mpDownloadWidget(NULL)
    , mDownloadInProgress(false)
{
    setupPalettes();
    connectPalettes();
}


WBBoardPaletteManager::~WBBoardPaletteManager()
{

}

void WBBoardPaletteManager::initPalettesPosAtStartup()
{
    mStylusPalette->initPosition();
}

void WBBoardPaletteManager::setupLayout()
{

}

/**
 * \brief Set up the dock palette widgets
 */
void WBBoardPaletteManager::setupDockPaletteWidgets()
{
    // Create the widgets for the dock palettes
    mpPageNavigWidget = new WBPageNavigationWidget();

    mpCachePropWidget = new WBCachePropertiesWidget();

    mpDownloadWidget = new WBDockDownloadWidget();
    // Add the dock palettes
    mLeftPalette = new WBLeftPalette(mContainer);

    // LEFT palette widgets
    mLeftPalette->registerWidget(mpPageNavigWidget);
    mLeftPalette->addTab(mpPageNavigWidget);

    mLeftPalette->connectSignals();
    mLeftPalette->showTabWidget(0);

    mRightPalette = new WBRightPalette(mContainer);
    // RIGHT palette widgets
    mpFeaturesWidget = new WBFeaturesWidget();
    mRightPalette->registerWidget(mpFeaturesWidget);
    mRightPalette->addTab(mpFeaturesWidget);

    // The cache widget will be visible only if a cache is put on the page
    mRightPalette->registerWidget(mpCachePropWidget);

    //  The download widget will be part of the right palette but
    //  will become visible only when the first download starts
    mRightPalette->registerWidget(mpDownloadWidget);
    mRightPalette->connectSignals();
    changeMode(eWBDockPaletteWidget_BOARD, true);

    // Hide the tabs that must be hidden
    mRightPalette->removeTab(mpDownloadWidget);
    mRightPalette->removeTab(mpCachePropWidget);

}

void WBBoardPaletteManager::slot_changeMainMode(WBApplicationController::MainMode mainMode)
{
//    Board = 0, Internet, Document, Tutorial, WebDocument

    switch( mainMode )
    {
        case WBApplicationController::Board:
            {
                // call changeMode only when switch NOT from desktop mode
                if(!WBApplication::applicationController->isShowingDesktop())
                    changeMode(eWBDockPaletteWidget_BOARD);
            }
            break;
        case WBApplicationController::Internet:
            changeMode(eWBDockPaletteWidget_WEB);
            break;

        case WBApplicationController::Document:
            changeMode(eWBDockPaletteWidget_DOCUMENT);
            break;

        default:
            {
                if (WBPlatformUtils::hasVirtualKeyboard() && mKeyboardPalette != NULL)
                    mKeyboardPalette->hide();
            }
            break;
    }
}

void WBBoardPaletteManager::slot_changeDesktopMode(bool isDesktop)
{
    WBApplicationController::MainMode currMode = WBApplication::applicationController->displayMode();
    if(!isDesktop)
    {
        switch( currMode )
        {
            case WBApplicationController::Board:
                changeMode(eWBDockPaletteWidget_BOARD);
                break;

            default:
                break;
        }
    }
    else
        changeMode(eWBDockPaletteWidget_DESKTOP);
}

void WBBoardPaletteManager::setupPalettes()
{
    if (WBPlatformUtils::hasVirtualKeyboard())
    {
        mKeyboardPalette = new WBKeyboardPalette(0);
#ifndef Q_OS_WIN
        connect(mKeyboardPalette, SIGNAL(closed()), mKeyboardPalette, SLOT(onDeactivated()));
#endif
    }

    setupDockPaletteWidgets();

    // Add the other palettes
    mStylusPalette = new WBStylusPalette(mContainer, WBSettings::settings()->appToolBarOrientationVertical->get().toBool() ? Qt::Vertical : Qt::Horizontal);
    connect(mStylusPalette, SIGNAL(stylusToolDoubleClicked(int)), WBApplication::boardController, SLOT(stylusToolDoubleClicked(int)));
    mStylusPalette->show(); // always show stylus palette at startup

    mZoomPalette = new WBZoomPalette(mContainer);

    mStylusPalette->stackUnder(mZoomPalette);

    //mTipPalette = new WBStartupHintsPalette(mContainer);
    QList<QAction*> backgroundsActions;

    backgroundsActions << WBApplication::mainWindow->actionPlainLightBackground;
    backgroundsActions << WBApplication::mainWindow->actionCrossedLightBackground;
    backgroundsActions << WBApplication::mainWindow->actionRuledLightBackground;
    backgroundsActions << WBApplication::mainWindow->actionPlainDarkBackground;
    backgroundsActions << WBApplication::mainWindow->actionCrossedDarkBackground;
    backgroundsActions << WBApplication::mainWindow->actionRuledDarkBackground;

    mBackgroundsPalette = new WBBackgroundPalette(backgroundsActions, mContainer);
    mBackgroundsPalette->setButtonIconSize(QSize(128, 128));
    mBackgroundsPalette->groupActions();
    mBackgroundsPalette->setClosable(true);
    mBackgroundsPalette->setAutoClose(false);
    mBackgroundsPalette->adjustSizeAndPosition();
    mBackgroundsPalette->hide();

    QList<QAction*> addItemActions;

    addItemActions << WBApplication::mainWindow->actionAddItemToCurrentPage;
    addItemActions << WBApplication::mainWindow->actionAddItemToNewPage;
    addItemActions << WBApplication::mainWindow->actionAddItemToLibrary;

    mAddItemPalette = new WBActionPalette(addItemActions, Qt::Horizontal, mContainer);
    mAddItemPalette->setButtonIconSize(QSize(128, 128));
    mAddItemPalette->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    mAddItemPalette->groupActions();
    mAddItemPalette->setClosable(true);
    mAddItemPalette->adjustSizeAndPosition();
    mAddItemPalette->hide();

    QList<QAction*> eraseActions;

    eraseActions << WBApplication::mainWindow->actionEraseAnnotations;
    eraseActions << WBApplication::mainWindow->actionEraseItems;
    eraseActions << WBApplication::mainWindow->actionClearPage;
    eraseActions << WBApplication::mainWindow->actionEraseBackground;

    mErasePalette = new WBActionPalette(eraseActions, Qt::Horizontal , mContainer);
    mErasePalette->setButtonIconSize(QSize(128, 128));
    mErasePalette->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    mErasePalette->groupActions();
    mErasePalette->setClosable(true);
    mErasePalette->adjustSizeAndPosition();
    mErasePalette->hide();

    QList<QAction*> pageActions;

    pageActions << WBApplication::mainWindow->actionNewPage;
    pageActions << WBApplication::mainWindow->actionDuplicatePage;
    pageActions << WBApplication::mainWindow->actionImportPage;

    mPagePalette = new WBActionPalette(pageActions, Qt::Horizontal , mContainer);
    mPagePalette->setButtonIconSize(QSize(128, 128));
    mPagePalette->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    mPagePalette->groupActions();
    mPagePalette->setClosable(true);
    mPagePalette->adjustSizeAndPosition();
    mPagePalette->hide();

    connect(WBSettings::settings()->appToolBarOrientationVertical, SIGNAL(changed(QVariant)), this, SLOT(changeStylusPaletteOrientation(QVariant)));
}

void WBBoardPaletteManager::pagePaletteButtonPressed()
{
    mPageButtonPressedTime = QTime::currentTime();

    mPendingPageButtonPressed = true;
    QTimer::singleShot(1000, this, SLOT(pagePaletteButtonReleased()));
}


void WBBoardPaletteManager::pagePaletteButtonReleased()
{
    if (mPendingPageButtonPressed)
    {
        if( mPageButtonPressedTime.msecsTo(QTime::currentTime()) > 900)
        {
            // The palette is reinstanciated because the duplication depends on the current scene
            delete(mPagePalette);
            mPagePalette = 0;
            QList<QAction*>pageActions;
            pageActions << WBApplication::mainWindow->actionNewPage;
            pageActions << WBApplication::mainWindow->actionDuplicatePage;
            pageActions << WBApplication::mainWindow->actionImportPage;

            mPagePalette = new WBActionPalette(pageActions, Qt::Horizontal , mContainer);
            mPagePalette->setButtonIconSize(QSize(128, 128));
            mPagePalette->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
            mPagePalette->groupActions();
            mPagePalette->setClosable(true);

            // As we recreate the pagePalette every time, we must reconnect the slots
            connect(WBApplication::mainWindow->actionNewPage, SIGNAL(triggered()), mPagePalette, SLOT(close()));
            connect(WBApplication::mainWindow->actionDuplicatePage, SIGNAL(triggered()), mPagePalette, SLOT(close()));
            connect(WBApplication::mainWindow->actionImportPage, SIGNAL(triggered()), mPagePalette, SLOT(close()));
            connect(mPagePalette, SIGNAL(closed()), this, SLOT(pagePaletteClosed()));

            togglePagePalette(true);
        }
        else
        {
            WBApplication::mainWindow->actionNewPage->trigger();
        }

        mPendingPageButtonPressed = false;
    }
}

void WBBoardPaletteManager::erasePaletteButtonPressed()
{
    mEraseButtonPressedTime = QTime::currentTime();

    mPendingEraseButtonPressed = true;
    QTimer::singleShot(1000, this, SLOT(erasePaletteButtonReleased()));
}


void WBBoardPaletteManager::erasePaletteButtonReleased()
{
    if (mPendingEraseButtonPressed)
    {
        if( mEraseButtonPressedTime.msecsTo(QTime::currentTime()) > 900)
        {
            toggleErasePalette(true);
        }
        else
        {
            WBApplication::mainWindow->actionClearPage->trigger();
        }

        mPendingEraseButtonPressed = false;
    }
}

void WBBoardPaletteManager::linkClicked(const QUrl& url)
{
      WBApplication::applicationController->showInternet();
      WBApplication::webController->loadUrl(url);
}

void WBBoardPaletteManager::purchaseLinkActivated(const QString& link)
{
    WBApplication::applicationController->showInternet();
    WBApplication::webController->loadUrl(QUrl(link));
}

void WBBoardPaletteManager::connectPalettes()
{
    connect(WBApplication::mainWindow->actionStylus, SIGNAL(toggled(bool)), this, SLOT(toggleStylusPalette(bool)));

    foreach(QWidget *widget, WBApplication::mainWindow->actionZoomIn->associatedWidgets())
    {
        QAbstractButton *button = qobject_cast<QAbstractButton*>(widget);
        if (button)
        {
            connect(button, SIGNAL(pressed()), this, SLOT(zoomButtonPressed()));
            connect(button, SIGNAL(released()), this, SLOT(zoomButtonReleased()));
        }
    }

    foreach(QWidget *widget, WBApplication::mainWindow->actionZoomOut->associatedWidgets())
    {
        QAbstractButton *button = qobject_cast<QAbstractButton*>(widget);
        if (button)
        {
            connect(button, SIGNAL(pressed()), this, SLOT(zoomButtonPressed()));
            connect(button, SIGNAL(released()), this, SLOT(zoomButtonReleased()));
        }
    }

    foreach(QWidget *widget, WBApplication::mainWindow->actionHand->associatedWidgets())
    {
        QAbstractButton *button = qobject_cast<QAbstractButton*>(widget);
        if (button)
        {
            connect(button, SIGNAL(pressed()), this, SLOT(panButtonPressed()));
            connect(button, SIGNAL(released()), this, SLOT(panButtonReleased()));
        }
    }

    connect(WBApplication::mainWindow->actionBackgrounds, SIGNAL(toggled(bool)), this, SLOT(toggleBackgroundPalette(bool)));
    connect(mBackgroundsPalette, SIGNAL(closed()), this, SLOT(backgroundPaletteClosed()));

    connect(WBApplication::mainWindow->actionPlainLightBackground, SIGNAL(triggered()), this, SLOT(changeBackground()));
    connect(WBApplication::mainWindow->actionCrossedLightBackground, SIGNAL(triggered()), this, SLOT(changeBackground()));
    connect(WBApplication::mainWindow->actionRuledLightBackground, SIGNAL(triggered()), this, SLOT(changeBackground()));
    connect(WBApplication::mainWindow->actionPlainDarkBackground, SIGNAL(triggered()), this, SLOT(changeBackground()));
    connect(WBApplication::mainWindow->actionCrossedDarkBackground, SIGNAL(triggered()), this, SLOT(changeBackground()));
    connect(WBApplication::mainWindow->actionRuledDarkBackground, SIGNAL(triggered()), this, SLOT(changeBackground()));
    connect(WBApplication::mainWindow->actionPodcast, SIGNAL(triggered(bool)), this, SLOT(tooglePodcastPalette(bool)));

    connect(WBApplication::mainWindow->actionAddItemToCurrentPage, SIGNAL(triggered()), this, SLOT(addItemToCurrentPage()));
    connect(WBApplication::mainWindow->actionAddItemToNewPage, SIGNAL(triggered()), this, SLOT(addItemToNewPage()));
    connect(WBApplication::mainWindow->actionAddItemToLibrary, SIGNAL(triggered()), this, SLOT(addItemToLibrary()));

    connect(WBApplication::mainWindow->actionEraseItems, SIGNAL(triggered()), mErasePalette, SLOT(close()));
    connect(WBApplication::mainWindow->actionEraseAnnotations, SIGNAL(triggered()), mErasePalette, SLOT(close()));
    connect(WBApplication::mainWindow->actionClearPage, SIGNAL(triggered()), mErasePalette, SLOT(close()));
    connect(WBApplication::mainWindow->actionEraseBackground,SIGNAL(triggered()),mErasePalette,SLOT(close()));
    connect(mErasePalette, SIGNAL(closed()), this, SLOT(erasePaletteClosed()));

    foreach(QWidget *widget, WBApplication::mainWindow->actionErase->associatedWidgets())
    {
        QAbstractButton *button = qobject_cast<QAbstractButton*>(widget);
        if (button)
        {
            connect(button, SIGNAL(pressed()), this, SLOT(erasePaletteButtonPressed()));
            connect(button, SIGNAL(released()), this, SLOT(erasePaletteButtonReleased()));
        }
    }

    connect(WBApplication::mainWindow->actionNewPage, SIGNAL(triggered()), mPagePalette, SLOT(close()));
    connect(WBApplication::mainWindow->actionDuplicatePage, SIGNAL(triggered()), mPagePalette, SLOT(close()));
    connect(WBApplication::mainWindow->actionImportPage, SIGNAL(triggered()), mPagePalette, SLOT(close()));
    connect(mPagePalette, SIGNAL(closed()), this, SLOT(pagePaletteClosed()));

    foreach(QWidget *widget, WBApplication::mainWindow->actionPages->associatedWidgets())
    {
        QAbstractButton *button = qobject_cast<QAbstractButton*>(widget);
        if (button)
        {
            connect(button, SIGNAL(pressed()), this, SLOT(pagePaletteButtonPressed()));
            connect(button, SIGNAL(released()), this, SLOT(pagePaletteButtonReleased()));
        }
    }
}


bool isFirstResized = true;
void WBBoardPaletteManager::containerResized()
{
    int innerMargin = WBSettings::boardMargin;

    int userLeft = innerMargin;
    int userWidth = mContainer->width() - (2 * innerMargin);
    int userTop = innerMargin;
    int userHeight = mContainer->height() - (2 * innerMargin);

    if(mStylusPalette)
    {
        mStylusPalette->move(userLeft, userTop);
        mStylusPalette->adjustSizeAndPosition();
        mStylusPalette->initPosition();
    }

    if(mZoomPalette)
    {
        mZoomPalette->move(userLeft + userWidth - mZoomPalette->width()
                , userTop + userHeight /*- mPageNumberPalette->height()*/ - innerMargin - mZoomPalette->height());
        mZoomPalette->adjustSizeAndPosition();
        mZoomPalette->refreshPalette();
    }

    if (isFirstResized && mKeyboardPalette && mKeyboardPalette->parent() == WBApplication::boardController->controlContainer())
    {
        isFirstResized = false;
        mKeyboardPalette->move(userLeft + (userWidth - mKeyboardPalette->width())/2,
                               userTop + (userHeight - mKeyboardPalette->height())/2);
        mKeyboardPalette->adjustSizeAndPosition();
    }

    if(mLeftPalette)
	{
        mLeftPalette->resize(mLeftPalette->width(), mContainer->height());
        mLeftPalette->resize(mLeftPalette->width(), mContainer->height());
    }

    if(mRightPalette)
    {
        mRightPalette->resize(mRightPalette->width(), mContainer->height());
        mRightPalette->resize(mRightPalette->width(), mContainer->height());
    }
}


void WBBoardPaletteManager::changeBackground()
{
    if (WBApplication::mainWindow->actionCrossedLightBackground->isChecked())
        WBApplication::boardController->changeBackground(false, WBPageBackground::crossed);

    else if (WBApplication::mainWindow->actionRuledLightBackground->isChecked())
        WBApplication::boardController->changeBackground(false, WBPageBackground::ruled);

    else if (WBApplication::mainWindow->actionPlainDarkBackground->isChecked())
        WBApplication::boardController->changeBackground(true, WBPageBackground::plain);

    else if (WBApplication::mainWindow->actionCrossedDarkBackground->isChecked())
        WBApplication::boardController->changeBackground(true, WBPageBackground::crossed);

    else if (WBApplication::mainWindow->actionRuledDarkBackground->isChecked())
        WBApplication::boardController->changeBackground(true, WBPageBackground::ruled);

    else
        WBApplication::boardController->changeBackground(false, WBPageBackground::plain);

    mBackgroundsPalette->backgroundChanged();
}


void WBBoardPaletteManager::activeSceneChanged()
{
    WBGraphicsScene *activeScene =  WBApplication::boardController->activeScene();
    int pageIndex = WBApplication::boardController->activeSceneIndex();

    if (mStylusPalette)
        connect(mStylusPalette, SIGNAL(mouseEntered()), activeScene, SLOT(hideTool()));

    if (mpPageNavigWidget)
    {
        mpPageNavigWidget->setPageNumber(WBDocumentContainer::pageFromSceneIndex(pageIndex), activeScene->document()->pageCount());
    }

    if (mZoomPalette)
        connect(mZoomPalette, SIGNAL(mouseEntered()), activeScene, SLOT(hideTool()));

    if (mBackgroundsPalette) {
        connect(mBackgroundsPalette, SIGNAL(mouseEntered()), activeScene, SLOT(hideTool()));
        mBackgroundsPalette->refresh();
    }
}


void WBBoardPaletteManager::toggleBackgroundPalette(bool checked)
{
    mBackgroundsPalette->setVisible(checked);

    if (checked)
    {
        WBApplication::mainWindow->actionErase->setChecked(false);
        WBApplication::mainWindow->actionNewPage->setChecked(false);

        mBackgroundsPalette->adjustSizeAndPosition();
        mBackgroundsPalette->move((mContainer->width() - mBackgroundsPalette->width()) / 2,
            (mContainer->height() - mBackgroundsPalette->height()) / 5);
    }
}


void WBBoardPaletteManager::backgroundPaletteClosed()
{
    WBApplication::mainWindow->actionBackgrounds->setChecked(false);
}


void WBBoardPaletteManager::toggleStylusPalette(bool checked)
{
    mStylusPalette->setVisible(checked);
}


void WBBoardPaletteManager::toggleErasePalette(bool checked)
{
    mErasePalette->setVisible(checked);
    if (checked)
    {
        WBApplication::mainWindow->actionBackgrounds->setChecked(false);
        WBApplication::mainWindow->actionNewPage->setChecked(false);

        mErasePalette->adjustSizeAndPosition();
        mErasePalette->move((mContainer->width() - mErasePalette->width()) / 2,
            (mContainer->height() - mErasePalette->height()) / 5);
    }
}


void WBBoardPaletteManager::erasePaletteClosed()
{
    WBApplication::mainWindow->actionErase->setChecked(false);
}


void WBBoardPaletteManager::togglePagePalette(bool checked)
{
    mPagePalette->setVisible(checked);
    if (checked)
    {
        WBApplication::mainWindow->actionBackgrounds->setChecked(false);
        WBApplication::mainWindow->actionErase->setChecked(false);

        mPagePalette->adjustSizeAndPosition();
        mPagePalette->move((mContainer->width() - mPagePalette->width()) / 2,
            (mContainer->height() - mPagePalette->height()) / 5);
    }
}


void WBBoardPaletteManager::pagePaletteClosed()
{
    WBApplication::mainWindow->actionPages->setChecked(false);
}


void WBBoardPaletteManager::tooglePodcastPalette(bool checked)
{
    WBPodcastController::instance()->toggleRecordingPalette(checked);
}


void WBBoardPaletteManager::addItem(const QUrl& pUrl)
{
    mItemUrl = pUrl;
    mPixmap = QPixmap();
    mPos = QPointF(0, 0);
    mScaleFactor = 1.;

    mAddItemPalette->show();
    mAddItemPalette->adjustSizeAndPosition();

    mAddItemPalette->move((mContainer->width() - mAddItemPalette->width()) / 2,
        (mContainer->height() - mAddItemPalette->height()) / 5);
}

void WBBoardPaletteManager::changeMode(eWBDockPaletteWidgetMode newMode, bool isInit)
{
    bool rightPaletteVisible = mRightPalette->switchMode(newMode);
    bool leftPaletteVisible = mLeftPalette->switchMode(newMode);

    switch( newMode )
    {
        case eWBDockPaletteWidget_BOARD:
            {
                // On Application start up the mAddItemPalette isn't initialized yet
                if(mAddItemPalette){
                    mAddItemPalette->setParent(WBApplication::boardController->controlContainer());
                }
                mLeftPalette->assignParent(mContainer);
                mRightPalette->assignParent(mContainer);
                mRightPalette->stackUnder(mStylusPalette);
                mLeftPalette->stackUnder(mStylusPalette);
                if (WBPlatformUtils::hasVirtualKeyboard()
                    && mKeyboardPalette != NULL
                    && WBSettings::settings()->useSystemOnScreenKeyboard->get().toBool() == false)
                {
                    if(mKeyboardPalette->m_isVisible) {
                        mKeyboardPalette->hide();
                        mKeyboardPalette->setParent(WBApplication::boardController->controlContainer());
                        mKeyboardPalette->show();
                    }
                    else
                        mKeyboardPalette->setParent(WBApplication::boardController->controlContainer());
                }

                mLeftPalette->setVisible(leftPaletteVisible);
                mRightPalette->setVisible(rightPaletteVisible);
#ifdef Q_OS_WIN
                if (rightPaletteVisible)
                    mRightPalette->setAdditionalVOffset(0);
#endif

                if( !isInit )
                    containerResized();
                if (mWebToolsCurrentPalette)
                    mWebToolsCurrentPalette->hide();
            }
            break;

        case eWBDockPaletteWidget_DESKTOP:
            {
                mAddItemPalette->setParent((QWidget*)WBApplication::applicationController->uninotesController()->drawingView());
                mLeftPalette->assignParent((QWidget*)WBApplication::applicationController->uninotesController()->drawingView());
                mRightPalette->assignParent((QWidget*)WBApplication::applicationController->uninotesController()->drawingView());
                mStylusPalette->raise();

                if (WBPlatformUtils::hasVirtualKeyboard()
                    && mKeyboardPalette != NULL
                    && WBSettings::settings()->useSystemOnScreenKeyboard->get().toBool() == false)
                {

                    if(mKeyboardPalette->m_isVisible)
                    {
                        mKeyboardPalette->hide();
#ifndef Q_OS_LINUX
                        mKeyboardPalette->setParent((QWidget*)WBApplication::applicationController->uninotesController()->drawingView());
#else
                        mKeyboardPalette->setParent(0);
#endif
#ifdef Q_OS_OSX
                        mKeyboardPalette->setWindowFlags(Qt::Dialog | Qt::Popup | Qt::FramelessWindowHint);
#endif
                        mKeyboardPalette->show();
                    }
                    else
// In linux keyboard in desktop mode have to allways be with null parent
#ifdef Q_OS_LINUX
                        mKeyboardPalette->setParent(0);
#else
                        mKeyboardPalette->setParent((QWidget*)WBApplication::applicationController->uninotesController()->drawingView());
#endif //Q_OS_LINUX
#ifdef Q_OS_OSX
                        mKeyboardPalette->setWindowFlags(Qt::Dialog | Qt::Popup | Qt::FramelessWindowHint);
#endif

                }

                mLeftPalette->setVisible(leftPaletteVisible);
                mRightPalette->setVisible(rightPaletteVisible);
#ifdef Q_OS_WIN
                if (rightPaletteVisible && WBSettings::settings()->appToolBarPositionedAtTop->get().toBool())
                    mRightPalette->setAdditionalVOffset(30);
#endif

                if(!isInit)
                    WBApplication::applicationController->uninotesController()->TransparentWidgetResized();

                if (mWebToolsCurrentPalette)
                    mWebToolsCurrentPalette->hide();
            }
            break;

        case eWBDockPaletteWidget_WEB:
            {
                mAddItemPalette->setParent(WBApplication::mainWindow);
                if (WBPlatformUtils::hasVirtualKeyboard()
                    && mKeyboardPalette != NULL
                    && WBSettings::settings()->useSystemOnScreenKeyboard->get().toBool() == false)
                {
//                    tmp variable?
//                    WBBrowserWindow* brWnd = WBApplication::webController->GetCurrentWebBrowser();

                    if(mKeyboardPalette->m_isVisible)
                    {
                        mKeyboardPalette->hide();
                        mKeyboardPalette->setParent(WBApplication::mainWindow);
                        mKeyboardPalette->show();
                    }
                    else
                        mKeyboardPalette->setParent(WBApplication::mainWindow);
                }

            }
            break;

        case eWBDockPaletteWidget_DOCUMENT:
            {
                mLeftPalette->setVisible(leftPaletteVisible);
                mRightPalette->setVisible(rightPaletteVisible);
                mLeftPalette->assignParent(WBApplication::documentController->controlView());
                mRightPalette->assignParent(WBApplication::documentController->controlView());
                if (WBPlatformUtils::hasVirtualKeyboard()
                    && mKeyboardPalette != NULL
                    && WBSettings::settings()->useSystemOnScreenKeyboard->get().toBool() == false)
                {

                    if(mKeyboardPalette->m_isVisible)
                    {
                        mKeyboardPalette->hide();
                        mKeyboardPalette->setParent(WBApplication::documentController->controlView());
                        mKeyboardPalette->show();
                    }
                    else
                        mKeyboardPalette->setParent(WBApplication::documentController->controlView());
                }
                if (mWebToolsCurrentPalette)
                    mWebToolsCurrentPalette->hide();
            }
            break;

        default:
            {
                mLeftPalette->setVisible(leftPaletteVisible);
                mRightPalette->setVisible(rightPaletteVisible);
                mLeftPalette->assignParent(0);
                mRightPalette->assignParent(0);
                if (WBPlatformUtils::hasVirtualKeyboard()
                    && mKeyboardPalette != NULL
                    && WBSettings::settings()->useSystemOnScreenKeyboard->get().toBool() == false)
                {

                    if(mKeyboardPalette->m_isVisible)
                    {
                        mKeyboardPalette->hide();
                        mKeyboardPalette->setParent(0);
                        mKeyboardPalette->show();
                    }
                    else
                        mKeyboardPalette->setParent(0);
                }
            }
            break;
    }

    if( !isInit )
        WBApplication::boardController->notifyPageChanged();
}

void WBBoardPaletteManager::addItem(const QPixmap& pPixmap, const QPointF& pos,  qreal scaleFactor, const QUrl& sourceUrl)
{
    mItemUrl = sourceUrl;
    mPixmap = pPixmap;
    mPos = pos;
    mScaleFactor = scaleFactor;

    mAddItemPalette->show();
    mAddItemPalette->adjustSizeAndPosition();

    mAddItemPalette->move((mContainer->width() - mAddItemPalette->width()) / 2,
        (mContainer->height() - mAddItemPalette->height()) / 5);
}


void WBBoardPaletteManager::addItemToCurrentPage()
{
    WBApplication::applicationController->showBoard();
    mAddItemPalette->hide();
    if(mPixmap.isNull())
        WBApplication::boardController->downloadURL(mItemUrl);
    else
    {
        WBGraphicsPixmapItem* item = WBApplication::boardController->activeScene()->addPixmap(mPixmap, NULL, mPos, mScaleFactor);

        QString documentPath = WBApplication::boardController->selectedDocument()->persistencePath();
        QString fileName = WBPersistenceManager::imageDirectory + "/" + item->uuid().toString() + ".png";
        QString path = documentPath + "/" + fileName;

        item->setSourceUrl(QUrl(path));
        item->setSelected(true);

        WBDrawingController::drawingController()->setStylusTool(WBStylusTool::Selector);
    }
}


void WBBoardPaletteManager::addItemToNewPage()
{
    WBApplication::boardController->addScene();
    addItemToCurrentPage();
}


void WBBoardPaletteManager::addItemToLibrary()
{
    if(mPixmap.isNull())
    {
       mPixmap = QPixmap(mItemUrl.toLocalFile());
    }

    if(!mPixmap.isNull())
    {
        if(mScaleFactor != 1.)
        {
             mPixmap = mPixmap.scaled(mScaleFactor * mPixmap.width(), mScaleFactor* mPixmap.height()
                     , Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        QImage image = mPixmap.toImage();

        QDateTime now = QDateTime::currentDateTime();
        QString capturedName  = tr("CapturedImage") + "-" + now.toString("dd-MM-yyyy hh-mm-ss") + ".png";
        mpFeaturesWidget->importImage(image, capturedName);
    }
    else
    {
        WBApplication::showMessage(tr("Error Adding Image to Library"));
    }

    mAddItemPalette->hide();
}

void WBBoardPaletteManager::zoomButtonPressed()
{
    mZoomButtonPressedTime = QTime::currentTime();

    mPendingZoomButtonPressed = true;
    QTimer::singleShot(1000, this, SLOT(zoomButtonReleased()));
}


void WBBoardPaletteManager::zoomButtonReleased()
{
    if (mPendingZoomButtonPressed)
    {
        if(mZoomButtonPressedTime.msecsTo(QTime::currentTime()) > 900)
        {
            mBoardControler->zoomRestore();
        }

        mPendingZoomButtonPressed = false;
    }
}

void WBBoardPaletteManager::panButtonPressed()
{
    mPanButtonPressedTime = QTime::currentTime();

    mPendingPanButtonPressed = true;
    QTimer::singleShot(1000, this, SLOT(panButtonReleased()));
}


void WBBoardPaletteManager::panButtonReleased()
{
    if (mPendingPanButtonPressed)
    {
        if(mPanButtonPressedTime.msecsTo(QTime::currentTime()) > 900)
        {
            mBoardControler->centerRestore();
        }

        mPendingPanButtonPressed = false;
    }
}

void WBBoardPaletteManager::showVirtualKeyboard(bool show)
{
    if (mKeyboardPalette)
        mKeyboardPalette->setVisible(show);
}

void WBBoardPaletteManager::changeStylusPaletteOrientation(QVariant var)
{
    bool bVertical = var.toBool();
    bool bVisible = mStylusPalette->isVisible();

    // Clean the old palette
    if(NULL != mStylusPalette)
    {
        delete mStylusPalette;
        mStylusPalette = NULL;
    }

    // Create the new palette
    if(bVertical)
    {
        mStylusPalette = new WBStylusPalette(mContainer, Qt::Vertical);
    }
    else
    {
        mStylusPalette = new WBStylusPalette(mContainer, Qt::Horizontal);
    }

    connect(mStylusPalette, SIGNAL(stylusToolDoubleClicked(int)), WBApplication::boardController, SLOT(stylusToolDoubleClicked(int)));
    mStylusPalette->setVisible(bVisible); // always show stylus palette at startup
}


void WBBoardPaletteManager::refreshPalettes()
{
    mRightPalette->update();
    mLeftPalette->update();
}

void WBBoardPaletteManager::startDownloads()
{
    if(!mDownloadInProgress)
    {
        mDownloadInProgress = true;
        mpDownloadWidget->setVisibleState(true);
        mRightPalette->addTab(mpDownloadWidget);
    }
}

void WBBoardPaletteManager::stopDownloads()
{
    if(mDownloadInProgress)
    {
        mDownloadInProgress = false;
        mpDownloadWidget->setVisibleState(false);
        mRightPalette->removeTab(mpDownloadWidget);
    }
}
