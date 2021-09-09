#ifndef WBTABWIDGET_H
#define WBTABWIDGET_H

#include <QtWidgets>
#include <QShortcut>
#include <QStackedWidget>
#include <QCompleter>
#include <QTabBar>
#include <QWebEnginePage>
#include <QTabWidget>
#include <QLineEdit>

class WBTabBar : public QTabBar
{
    Q_OBJECT;

public:
    WBTabBar(QWidget *parent = 0);

protected:
    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);

signals:
	void newTab();
	void cloneTab(int index);
	void closeTab(int index);
	void closeOtherTabs(int index);
	void reloadTab(int index);
	void reloadAllTabs();
	void tabMoveRequested(int fromIndex, int toIndex);

private slots:
    void selectTabAction();
    void cloneTab();
    void closeTab();
    void closeOtherTabs();
    void reloadTab();
    void contextMenuRequested(const QPoint &position);

private:
    QList<QShortcut*> mTabShortcuts;
    friend class WBTabWidget;

    QPoint mDragStartPos;
};

#include <QtWebEngine>


class WBWebView;

class WBWebActionMapper : public QObject
{
    Q_OBJECT;

    public:
        WBWebActionMapper(QAction *root, QWebEnginePage::WebAction webAction, QObject *parent);
		QWebEnginePage::WebAction webAction() const;
        void addChild(QAction *action);
        void updateCurrent(QWebEnginePage *currentParent);

    private slots:
        void rootTriggered();
        void childChanged();
        void rootDestroyed();
        void currentDestroyed();

    private:
		QWebEnginePage *mCurrentParent;
        QAction *mRootAction;
		QWebEnginePage::WebAction mWebAction;
};


class WBTabWidget : public QTabWidget
{
    Q_OBJECT

    signals:
        // tab widget signals
        void loadPage(const QString &url);

        // current tab signals
        void setCurrentTitle(const QString &url);
        void showStatusBarMessage(const QString &message);
        void linkHovered(const QString &link);
        void loadProgress(int progress);
        void loadFinished(bool pOk);
        void geometryChangeRequested(const QRect &geometry);
        void menuBarVisibilityChangeRequested(bool visible);
        void statusBarVisibilityChangeRequested(bool visible);
        void toolBarVisibilityChangeRequested(bool visible);
        void printRequested(QWebChannel *frame);

    public:
        WBTabWidget(QWidget *parent = 0);
        void clear();
        void addWebAction(QAction *action, QWebEnginePage::WebAction webAction);

        QWidget *lineEditStack() const;
        QLineEdit *currentLineEdit() const;
        WBWebView *currentWebView() const;
        WBWebView *webView(int index) const;
        QLineEdit *lineEdit(int index) const;
        int webViewIndex(WBWebView *webView) const;

        QByteArray saveState() const;
        bool restoreState(const QByteArray &state);

        WBTabBar* tabBar() { return mTabBar; }
        QStackedWidget* lineEdits() { return mLineEdits; }

        void setLineEditStackVisible(bool visible) {mLineEdits->setVisible(visible);mLineEdits->hide();}

    protected:
        void mouseDoubleClickEvent(QMouseEvent *event);
        void contextMenuEvent(QContextMenuEvent *event);
        void mouseReleaseEvent(QMouseEvent *event);
        void paintEvent(QPaintEvent * event);
        QRect addTabButtonRect();

    public slots:
        void loadUrlInCurrentTab(const QUrl &url);
        WBWebView *newTab(bool makeCurrent = true);
        void cloneTab(int index = -1);
        void closeTab(int index = -1);
        void closeOtherTabs(int index);
        void reloadTab(int index = -1);
        void reloadAllTabs();
        void nextTab();
        void previousTab();

    private slots:
        void currentChanged(int index);
        void aboutToShowRecentTabsMenu();
        void aboutToShowRecentTriggeredAction(QAction *action);
        void webViewLoadStarted();
        void webViewIconChanged();
        void webViewTitleChanged(const QString &title);
        void webViewUrlChanged(const QUrl &url);
        void lineEditReturnPressed();
        void windowCloseRequested();
        void moveTab(int fromIndex, int toIndex);

    private:
        QAction *mRecentlyClosedTabsAction;

        QMenu *mRecentlyClosedTabsMenu;
        static const int sRecentlyClosedTabsSize = 10;
        QList<QUrl> mRecentlyClosedTabs;
        QList<WBWebActionMapper*> mWebActions;

        QCompleter *mLineEditCompleter;
        QStackedWidget *mLineEdits;
        WBTabBar *mTabBar;
        QPixmap mAddTabIcon;
};

#endif // WBTABWIDGET_H

