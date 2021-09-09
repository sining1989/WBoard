#ifndef WBAPPLICATIONCONTROLLER_H_
#define WBAPPLICATIONCONTROLLER_H_

#include <QtWidgets>
#include <QtNetwork>
#include <QNetworkAccessManager>

#include <QtNetwork/QHttpPart>
#include <QJSEngine>

class WBBoardView;
class WBDocumentProxy;
class WBGraphicsScene;
class WBDesktopAnnotationController;
class WBScreenMirror;
class WBMainWindow;
class WBDisplayManager;
class WBVersion;
class WBSoftwareUpdate;
class QNetworkAccessManager;
class QNetworkReply;
class WBRightPalette;
class WBOpenSankoreImporter;

class WBApplicationController : public QObject
{
    Q_OBJECT

    public:
        WBApplicationController(WBBoardView *pControlView, WBBoardView *pDisplayView, WBMainWindow *pMainWindow, QObject* parent, WBRightPalette* rightPalette);
        virtual ~WBApplicationController();

        int initialHScroll() { return mInitialHScroll; }
        int initialVScroll() { return mInitialVScroll; }

        void adaptToolBar();
        void adjustDisplayView();
        void adjustPreviousViews(int pActiveSceneIndex, WBDocumentProxy *pActiveDocument);

        void blackout();

        void initScreenLayout(bool useMultiscreen);

        void closing();

        void setMirrorSourceWidget(QWidget*);

        void mirroringEnabled(bool);

        void initViewState(int horizontalPosition, int verticalPosition);

        void showBoard();

        void showInternet();

        void showDocument();

        void showMessage(const QString& message, bool showSpinningWheel);

        void importFile(const QString& pFilePath);

        WBDisplayManager* displayManager()
        {
            return mDisplayManager;
        }

        WBDesktopAnnotationController* uninotesController()
        {
            return mUninoteController;
        }

        enum MainMode
        {
            Board = 0, Internet, Document, WebDocument
        };

        MainMode displayMode()
        {
            return mMainMode;
        }

        bool isCheckingForSoftwareUpdate() const;

        bool isShowingDesktop()
        {
            return mIsShowingDesktop;
        }

        QStringList widgetInlineJavaScripts();

    signals:
        void mainModeChanged(WBApplicationController::MainMode pMode);
        void desktopMode(bool displayed);

    public slots:
        void addCapturedPixmap(const QPixmap &pPixmap, bool pageMode, const QUrl& sourceUrl = QUrl());

        void addCapturedEmbedCode(const QString& embedCode);

        void screenLayoutChanged();

        void showDesktop(bool dontSwitchFrontProcess = false);

        void hideDesktop();

        void useMultiScreen(bool use);

        void actionCut();
        void actionCopy();
        void actionPaste();

        void checkUpdateRequest();
        void checkAtLaunch();

    private slots:
        void updateRequestFinished(QNetworkReply * reply);

    protected:
        WBDesktopAnnotationController *mUninoteController;

        WBMainWindow *mMainWindow;

        WBOpenSankoreImporter *mOpenSankoreImporter;

        WBBoardView *mControlView;
        WBBoardView *mDisplayView;
        QList<WBBoardView*> mPreviousViews;

        WBGraphicsScene *mBlackScene;

        WBScreenMirror* mMirror;

        int mInitialHScroll, mInitialVScroll;

    private:
        MainMode mMainMode;

        WBDisplayManager *mDisplayManager;

        bool mAutomaticCheckForUpdates;
        bool mCheckingForUpdates;

        void setCheckingForUpdates(bool value);

        bool mIsShowingDesktop;

        bool isNoUpdateDisplayed;
        void checkUpdate(const QUrl &url = QUrl());
        QNetworkAccessManager * mNetworkAccessManager;

        void downloadJsonFinished(QString updateString);
};

#endif /* WBAPPLICATIONCONTROLLER_H_ */
