#ifndef WBBOARDPALETTEMANAGER_H_
#define WBBOARDPALETTEMANAGER_H_

#include <QtWidgets>
#include <QtWebEngine>

#include "gui/WBLeftPalette.h"
#include "gui/WBRightPalette.h"
#include "gui/WBPageNavigationWidget.h"
#include "gui/WBCachePropertiesWidget.h"
#include "gui/WBDockDownloadWidget.h"
#include "core/WBApplicationController.h"
#include "gui/WBFeaturesWidget.h"


class WBWebToolsPalette;
class WBStylusPalette;
class WBClockPalette;
class WBPageNumberPalette;
class WBZoomPalette;
class WBActionPalette;
class WBBackgroundPalette;
class WBBoardController;
class WBServerXMLHttpRequest;
class WBKeyboardPalette;
class WBMainWindow;
class WBApplicationController;
class WBStartupHintsPalette;

class WBBoardPaletteManager : public QObject
{
    Q_OBJECT

    public:
        WBBoardPaletteManager(QWidget* container, WBBoardController* controller);
        virtual ~WBBoardPaletteManager();

        void setupLayout();
        WBLeftPalette* leftPalette(){return mLeftPalette;}
        WBRightPalette* rightPalette(){return mRightPalette;}
        WBStylusPalette* stylusPalette(){return mStylusPalette;}
        WBActionPalette *addItemPalette() {return mAddItemPalette;}
        void showVirtualKeyboard(bool show = true);
        void initPalettesPosAtStartup();
        void refreshPalettes();

        WBKeyboardPalette *mKeyboardPalette;

        void setCurrentWebToolsPalette(WBWebToolsPalette *palette) {mWebToolsCurrentPalette = palette;}
        WBWebToolsPalette* mWebToolsCurrentPalette;

        void processPalettersWidget(WBDockPalette *paletter, eWBDockPaletteWidgetMode mode);
        void changeMode(eWBDockPaletteWidgetMode newMode, bool isInit = false);
        void startDownloads();
        void stopDownloads();

    public slots:
        void activeSceneChanged();
        void containerResized();
        void addItem(const QUrl& pUrl);
        void addItem(const QPixmap& pPixmap, const QPointF& p = QPointF(0.0, 0.0), qreal scale = 1.0, const QUrl& sourceUrl = QUrl());

        void slot_changeMainMode(WBApplicationController::MainMode);
        void slot_changeDesktopMode(bool);

        void toggleErasePalette(bool ckecked);
		
	private:
        void setupPalettes();
        void connectPalettes();
        void positionFreeDisplayPalette();
        void setupDockPaletteWidgets();

        QWidget* mContainer;
        WBBoardController *mBoardControler;

        WBStylusPalette *mStylusPalette;

        WBZoomPalette *mZoomPalette;
	    WBStartupHintsPalette* mTipPalette;
        WBLeftPalette* mLeftPalette;
        WBRightPalette* mRightPalette;

        WBBackgroundPalette *mBackgroundsPalette;
        WBActionPalette *mToolsPalette;
        WBActionPalette* mAddItemPalette;
        WBActionPalette* mErasePalette;
        WBActionPalette* mPagePalette;

        QUrl mItemUrl;
        QPixmap mPixmap;
        QPointF mPos;
        qreal mScaleFactor;

        QTime mPageButtonPressedTime;
        bool mPendingPageButtonPressed;

        QTime mZoomButtonPressedTime;
        bool mPendingZoomButtonPressed;

        QTime mPanButtonPressedTime;
        bool mPendingPanButtonPressed;

        QTime mEraseButtonPressedTime;
        bool mPendingEraseButtonPressed;

        WBPageNavigationWidget* mpPageNavigWidget;

        WBCachePropertiesWidget* mpCachePropWidget;

        WBFeaturesWidget *mpFeaturesWidget;

        WBDockDownloadWidget* mpDownloadWidget;

        bool mDownloadInProgress;

    private slots:
        void changeBackground();

        void toggleBackgroundPalette(bool checked);
        void backgroundPaletteClosed();

        void toggleStylusPalette(bool checked);
        void tooglePodcastPalette(bool checked);

        void erasePaletteButtonPressed();
        void erasePaletteButtonReleased();

        void erasePaletteClosed();

        void togglePagePalette(bool ckecked);
        void pagePaletteClosed();

        void pagePaletteButtonPressed();
        void pagePaletteButtonReleased();

        void addItemToCurrentPage();
        void addItemToNewPage();
        void addItemToLibrary();

        void purchaseLinkActivated(const QString&);

        void linkClicked(const QUrl& url);

        void zoomButtonPressed();
        void zoomButtonReleased();
        void panButtonPressed();
        void panButtonReleased();

        void changeStylusPaletteOrientation(QVariant var);
};

#endif /* WBBOARDPALETTEMANAGER_H_ */
