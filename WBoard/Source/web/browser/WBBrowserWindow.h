#ifndef WBBROWSERMAINWINDOW_H
#define WBBROWSERMAINWINDOW_H

#include <QtWidgets>
#include <QtWebEngine>

class WBChaseWidget;
class WBTabWidget;
class WBToolbarSearch;
class WBWebView;
class WBHistoryManager;
class WBDownloadManager_;
class UBCookieJar;

#include "ui_mainWindow.h"

class WBBrowserWindow : public QWidget
{
    Q_OBJECT;

public:
    WBBrowserWindow(QWidget *parent = 0, Ui::MainWindow* uniboardMainWindow = 0);
    ~WBBrowserWindow();
    QSize sizeHint() const;

public:
    static QUrl guessUrlFromString(const QString &url);
    WBTabWidget *tabWidget() const;
    WBWebView *currentTabWebView() const;

    //void adaptToolBar(bool wideRes);

    //static WBHistoryManager *historyManager();
    static UBCookieJar *cookieJar();
    static WBDownloadManager_ *downloadManager();

public slots:
    void loadPage(const QString &url);
    void slotHome();

    void loadUrl(const QUrl &url);
    void loadUrlInNewTab(const QUrl &url);

    WBWebView *createNewTab();

    WBWebView* paintWidget();

    void tabCurrentChanged(int);

    void bookmarks();
    void addBookmark();

    void showTabAtTop(bool attop);

    void aboutToShowBackMenu();
    void aboutToShowForwardMenu();
    void openActionUrl(QAction *action);

signals:
    void activeViewPageChanged();
    void activeViewChange(QWidget*);
    void mirroringEnabled(bool checked);

protected:
    void closeEvent(QCloseEvent *event);

private slots:

    void slotLoadProgress(int);
    void slotUpdateStatusbar(const QString &string);
    void slotUpdateWindowTitle(const QString &title = QString());

    void slotFileSaveAs();

    void slotViewZoomIn();
    void slotViewZoomOut();
    void slotViewResetZoom();
    void slotViewZoomTextOnly(bool enable);

    void slotWebSearch();
    void slotToggleInspector(bool enable);
    void slotSelectLineEdit();
    void slotSwapFocus();

    void geometryChangeRequested(const QRect &geometry);

private:
    static WBHistoryManager *sHistoryManager;
    static WBDownloadManager_ *sDownloadManager;

    void setupMenu();
    void setupToolBar();
    void updateStatusbarActionText(bool visible);

    QToolBar *mWebToolBar;
    //WBToolbarSearch *mSearchToolBar;
    WBChaseWidget *mChaseWidget;
    WBTabWidget *mTabWidget;

    //QAction *mSearchAction;

    QString mLastSearch;

    Ui::MainWindow* mUniboardMainWindow;

    QMenu    *mHistoryBackMenu;
    QMenu    *mHistoryForwardMenu;
};

#endif // WBBROWSERMAINWINDOW_H

