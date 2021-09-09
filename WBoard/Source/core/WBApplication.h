#ifndef WBAPPLICATION_H_
#define WBAPPLICATION_H_

#include <QtWidgets>
#include <QUndoStack>
#include <QToolBar>
#include <QMenu>

#include "qtsingleapplication/qtsingleapplication.h"

namespace Ui
{
    class MainWindow;
}

class WBBoardController;
class WBWebController;
class WBControlView;
class WBPreferencesController;
class WBResources;
class WBSettings;
class WBPersistenceManager;
class WBApplicationController;
class WBDocumentController;
class WBMainWindow;

class WBApplication : public QtSingleApplication
{
    Q_OBJECT

    public:
        WBApplication(const QString &id, int &argc, char **argv);
        virtual ~WBApplication();

        int exec(const QString& pFileToImport);

        void cleanup();

        static QPointer<QUndoStack> undoStack;

        static WBApplicationController *applicationController;
        static WBBoardController* boardController;
        static WBWebController* webController;
        static WBDocumentController* documentController;

        static WBMainWindow* mainWindow;

        static WBApplication* app()
        {
            return static_cast<WBApplication*>qApp;
        }

        static const QString mimeTypeUniboardDocument;
        static const QString mimeTypeUniboardPage;
        static const QString mimeTypeUniboardPageItem;
        static const QString mimeTypeUniboardPageThumbnail;

        static void showMessage(const QString& message, bool showSpinningWheel = false);
        static void setDisabled(bool disable);

        static QObject* staticMemoryCleaner;

        void decorateActionMenu(QAction* action);
        void insertSpaceToToolbarBeforeAction(QToolBar* toolbar, QAction* action, int width = -1);

        int toolBarHeight();
        bool eventFilter(QObject *obj, QEvent *event);

        bool isVerbose() { return mIsVerbose;}
        void setVerbose(bool verbose){mIsVerbose = verbose;}
        static QString urlFromHtml(QString html);
        static bool isFromWeb(QString url);

        static QScreen* controlScreen();
        static int controlScreenIndex();

    public slots:
        void showBoard();
        void showInternet();
        void showDocument();
        void startScript();
        void stopScript();

        void toolBarPositionChanged(QVariant topOrBottom);
        void toolBarDisplayTextChanged(QVariant display);

        void closeEvent(QCloseEvent *event);

        bool handleOpenMessage(const QString& pMessage);

    private slots:
        void closing();
//#ifdef Q_OS_OSX // for some reason this is not compiled if the ifdef is uncommented
        void showMinimized();
//#endif
        void onScreenCountChanged(int newCount);

    private:
        void updateProtoActionsState();
        void setupTranslators(QStringList args);
        QList<QMenu*> mProtoMenus;
        bool mIsVerbose;
        QString checkLanguageAvailabilityForSankore(QString& language);
    protected:
/*
#if defined(Q_OS_MAC) && !defined(QT_MAC_USE_COCOA)
        bool macEventFilter(EventHandlerCallRef caller, EventRef event);
#endif
        */

        WBPreferencesController* mPreferencesController;
        QTranslator* mApplicationTranslator;
        QTranslator* mQtGuiTranslator;

};

#endif /* WBAPPLICATION_H_ */
