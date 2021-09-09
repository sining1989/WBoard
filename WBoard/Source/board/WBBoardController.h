#ifndef WBBOARDCONTROLLER_H_
#define WBBOARDCONTROLLER_H_

#include <QtWidgets>

#include <QObject>
#include <QHBoxLayout>
#include <QUndoCommand>

#include "document/WBDocumentContainer.h"
#include "core/WBApplicationController.h"

class WBMainWindow;
class WBApplication;
class WBBoardView;

class WBDocumentController;
class WBMessageWindow;
class WBGraphicsScene;
class WBDocumentProxy;
class WBBlackoutWidget;
class WBToolWidget;
class WBVersion;
class WBSoftwareUpdate;
class WBSoftwareUpdateDialog;
class WBGraphicsMediaItem;
class WBGraphicsWidgetItem;
class WBBoardPaletteManager;
class WBItem;
class WBGraphicsItem;


class WBBoardController : public WBDocumentContainer
{
    Q_OBJECT

    public:
        enum SaveFlag {
            sf_none = 0x0,
            sf_showProgress = 0x1
        };
    Q_DECLARE_FLAGS(SaveFlags, SaveFlag)

    public:
        WBBoardController(WBMainWindow *mainWindow);
        virtual ~WBBoardController();

        void init();
        void setupLayout();

        WBGraphicsScene* activeScene() const;
        int activeSceneIndex() const;
        void setActiveSceneIndex(int i);
        QSize displayViewport();
        QSize controlViewport();
        QRectF controlGeometry();
        void closing();

        int currentPage();

        QWidget* controlContainer()
        {
            return mControlContainer;
        }

        WBBoardView* controlView()
        {
            return mControlView;
        }

        WBBoardView* displayView()
        {
            return mDisplayView;
        }

        WBGraphicsScene* activeScene()
        {
            return mActiveScene;
        }

        void setPenColorOnDarkBackground(const QColor& pColor)
        {
            if (mPenColorOnDarkBackground == pColor)
                return;

            mPenColorOnDarkBackground = pColor;
            emit penColorChanged();
        }

        void setPenColorOnLightBackground(const QColor& pColor)
        {
            if (mPenColorOnLightBackground == pColor)
                return;

            mPenColorOnLightBackground = pColor;
            emit penColorChanged();
        }

        void setMarkerColorOnDarkBackground(const QColor& pColor)
        {
            mMarkerColorOnDarkBackground = pColor;
        }

        void setMarkerColorOnLightBackground(const QColor& pColor)
        {
            mMarkerColorOnLightBackground = pColor;
        }

        QColor penColorOnDarkBackground()
        {
            return mPenColorOnDarkBackground;
        }

        QColor penColorOnLightBackground()
        {
            return mPenColorOnLightBackground;
        }

        QColor markerColorOnDarkBackground()
        {
            return mMarkerColorOnDarkBackground;
        }

        QColor markerColorOnLightBackground()
        {
            return mMarkerColorOnLightBackground;
        }

        qreal systemScaleFactor()
        {
            return mSystemScaleFactor;
        }
        qreal currentZoom();
        void persistViewPositionOnCurrentScene();
        void persistCurrentScene(bool isAnAutomaticBackup = false, bool forceImmediateSave = false);
        void showNewVersionAvailable(bool automatic, const WBVersion &installedVersion, const WBSoftwareUpdate &softwareUpdate);
        void setBoxing(QRect displayRect);
        void setToolbarTexts();
        static QUrl expandWidgetToTempDir(const QByteArray& pZipedData, const QString& pExtension = QString("wgt"));

        void setPageSize(QSize newSize);
        WBBoardPaletteManager *paletteManager()
        {
            return mPaletteManager;
        }

        void notifyCache(bool visible);
        void notifyPageChanged();
        void displayMetaData(QMap<QString, QString> metadatas);

        void findUniquesItems(const QUndoCommand *parent, QSet<QGraphicsItem *> &itms);
        void ClearUndoStack();

        void setActiveDocumentScene(WBDocumentProxy* pDocumentProxy, int pSceneIndex = 0, bool forceReload = false, bool onImport = false);
        void setActiveDocumentScene(int pSceneIndex);

        void moveSceneToIndex(int source, int target);
        void duplicateScene(int index);
        WBGraphicsItem *duplicateItem(WBItem *item);
        void deleteScene(int index);

        bool cacheIsVisible() {return mCacheWidgetIsEnabled;}

        QString actionGroupText(){ return mActionGroupText;}
        QString actionUngroupText(){ return mActionUngroupText;}

    public slots:
        void showDocumentsDialog();
        void showKeyboard(bool show);
        void togglePodcast(bool checked);
        void blackout();
        void addScene();
        void addScene(WBDocumentProxy* proxy, int sceneIndex, bool replaceActiveIfEmpty = false);
        void addScene(WBGraphicsScene* scene, bool replaceActiveIfEmpty = false);
        void duplicateScene();
        void importPage();
        void clearScene();
        void clearSceneItems();
        void clearSceneAnnotation();
        void clearSceneBackground();
        void zoomIn(QPointF scenePoint = QPointF(0,0));
        void zoomOut(QPointF scenePoint = QPointF(0,0));
        void zoomRestore();
        void centerRestore();
        void centerOn(QPointF scenePoint = QPointF(0,0));
        void zoom(const qreal ratio, QPointF scenePoint);
        void handScroll(qreal dx, qreal dy);
        void previousScene();
        void nextScene();
        void firstScene();
        void lastScene();
        void downloadURL(const QUrl& url, QString contentSourceUrl = QString(), const QPointF& pPos = QPointF(0.0, 0.0), const QSize& pSize = QSize(), bool isBackground = false, bool internalData = false);
        WBItem *downloadFinished(bool pSuccess, QUrl sourceUrl, QUrl contentUrl, QString pHeader,
                                 QByteArray pData, QPointF pPos, QSize pSize,
                                 bool isBackground = false, bool internalData = false);
        void changeBackground(bool isDark, WBPageBackground pageBackground);
        void setToolCursor(int tool);
        void showMessage(const QString& message, bool showSpinningWheel = false);
        void hideMessage();
        void setDisabled(bool disable);
        void setColorIndex(int pColorIndex);
        void removeTool(WBToolWidget* toolWidget);
        void hide();
        void show();
        void setWidePageSize(bool checked);
        void setRegularPageSize(bool checked);
        void stylusToolChanged(int tool);
        void grabScene(const QRectF& pSceneRect);
        WBGraphicsMediaItem* addVideo(const QUrl& pUrl, bool startPlay, const QPointF& pos, bool bUseSource = false);
        WBGraphicsMediaItem* addAudio(const QUrl& pUrl, bool startPlay, const QPointF& pos, bool bUseSource = false);
        WBGraphicsWidgetItem *addW3cWidget(const QUrl& pUrl, const QPointF& pos);
        void adjustDisplayViews();
        void cut();
        void copy();
        void paste();
        void processMimeData(const QMimeData* pMimeData, const QPointF& pPos);
        void moveGraphicsWidgetToControlView(WBGraphicsWidgetItem* graphicWidget);
        void moveToolWidgetToScene(WBToolWidget* toolWidget);
        void addItem();

        void freezeW3CWidgets(bool freeze);
        void freezeW3CWidget(QGraphicsItem* item, bool freeze);
        void startScript();
        void stopScript();

        void saveData(SaveFlags fls = sf_none);

        //void regenerateThumbnails();

    signals:
        void newPageAdded();
        void activeSceneChanged();
        void zoomChanged(qreal pZoomFactor);
        void penColorChanged();
        void controlViewportChanged();
        void backgroundChanged();
        void cacheEnabled();
        void documentReorganized(int index);
        void displayMetadata(QMap<QString, QString> metadata);
        void pageSelectionChanged(int index);
        void centerOnThumbnailRequired(int index);
        void npapiWidgetCreated(const QString &Url);

    protected:
        void setupViews();
        void setupToolbar();
        void connectToolbar();
        void initToolbarTexts();
        void updateActionStates();
        void updateSystemScaleFactor();
        QString truncate(QString text, int maxWidth);

    protected slots:
        void selectionChanged();
        void undoRedoStateChange(bool canUndo);
        void documentSceneChanged(WBDocumentProxy* proxy, int pIndex);

    private slots:
        void autosaveTimeout();
        void appMainModeChanged(WBApplicationController::MainMode);

    private:
        void initBackgroundGridSize();
        void updatePageSizeState();
        void saveViewState();
        int autosaveTimeoutFromSettings();

        WBMainWindow *mMainWindow;
        WBGraphicsScene* mActiveScene;
        int mActiveSceneIndex;
        WBBoardPaletteManager *mPaletteManager;
        WBSoftwareUpdateDialog *mSoftwareUpdateDialog;
        WBMessageWindow *mMessageWindow;
        WBBoardView *mControlView;
        WBBoardView *mDisplayView;
        QWidget *mControlContainer;
        QHBoxLayout *mControlLayout;
        qreal mZoomFactor;
        bool mIsClosing;
        QColor mPenColorOnDarkBackground;
        QColor mPenColorOnLightBackground;
        QColor mMarkerColorOnDarkBackground;
        QColor mMarkerColorOnLightBackground;
        qreal mSystemScaleFactor;
        bool mCleanupDone;
        QMap<QAction*, QPair<QString, QString> > mActionTexts;
        bool mCacheWidgetIsEnabled;
        QGraphicsItem* mLastCreatedItem;
        int mDeletingSceneIndex;
        int mMovingSceneIndex;
        QString mActionGroupText;
        QString mActionUngroupText;

        QTimer *mAutosaveTimer;

    private slots:
        void stylusToolDoubleClicked(int tool);
        void boardViewResized(QResizeEvent* event);
        void documentWillBeDeleted(WBDocumentProxy* pProxy);
        void updateBackgroundActionsState(bool isDark, WBPageBackground pageBackground);
        void colorPaletteChanged();
        void libraryDialogClosed(int ret);
        void lastWindowClosed();
        void onDownloadModalFinished();
};
#endif /* WBBOARDCONTROLLER_H_ */
