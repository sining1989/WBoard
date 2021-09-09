#ifndef WBWEBVIEW_H
#define WBWEBVIEW_H

#include <QtWidgets>
#include <QtWebEngine>

#include "WBWebTrapWebView.h"
#include "web/WBWebPage_.h"

class WBBrowserWindow;

class WBWebPage : public WBWebPage_
{
    Q_OBJECT;

public:
    WBWebPage(QObject *parent = 0);
    WBBrowserWindow *mainWindow();

protected:
    bool acceptNavigationRequest(QWebChannel *frame, const QNetworkRequest &request, NavigationType type);
	QWebEnginePage *createWindow(QWebEnginePage::WebWindowType type);
    QObject *createPlugin(const QString &classId, const QUrl &url, const QStringList &paramNames, const QStringList &paramValues);

    //bool supportsExtension(Extension extension) const {
    //    if (extension == QWebEnginePage::ErrorPageExtension)
    //    {
    //        return true;
    //    }
    //    return false;
    //}

    //bool extension(Extension extension, const ExtensionOption *option = 0, ExtensionReturn *output = 0)
    //{
    //    if (extension != QWebEnginePage::ErrorPageExtension)
    //        return false;

    //    ErrorPageExtensionOption *errorOption = (ErrorPageExtensionOption*) option;
    //    qDebug() << "Error loading " << qPrintable(errorOption->url.toString());
    //    if(errorOption->domain == QWebEnginePage::QtNetwork)
    //        qDebug() << "Network error (" << errorOption->error << "): ";
    //    else if(errorOption->domain == QWebEnginePage::Http)
    //        qDebug() << "HTTP error (" << errorOption->error << "): ";
    //    else if(errorOption->domain == QWebEnginePage::WebKit)
    //        qDebug() << "WebKit error (" << errorOption->error << "): ";

    //    qDebug() << qPrintable(errorOption->errorString);

    //    return false;
    //}

signals:
	void loadingUrl(const QUrl &url);

private slots:
    void handleUnsupportedContent(QNetworkReply *reply);

private:
    friend class WBWebView;

    // set the webview mousepressedevent
    Qt::KeyboardModifiers mKeyboardModifiers;
    Qt::MouseButtons mPressedButtons;
    bool mOpenInNewTab;
    QUrl mLoadingUrl;
};

class WBWebView : public WBWebTrapWebView
{
    Q_OBJECT

public:
    WBWebView(QWidget *parent = 0);
    WBWebPage *webPage() const { return mPage; }

    void load(const QUrl &url);
    void load ( const QNetworkRequest & request, QNetworkAccessManager::Operation operation = QNetworkAccessManager::GetOperation
            , const QByteArray & body = QByteArray());
    QUrl url() const;

    QString lastStatusBarText() const;
    inline int progress() const { return mProgress; }

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);
    void wheelEvent(QWheelEvent *event);

private slots:
    void setProgress(int progress);
    void loadFinished(bool ok);
    void loadStarted();

    void setStatusBarText(const QString &string);

    void downloadRequested(const QNetworkRequest &request);
    void openLinkInNewTab();

private:
    QString mLastStatusBarText;
    QUrl mInitialUrl;
    int mProgress;
    WBWebPage *mPage;
    QTime mLoadStartTime;
};

#endif //WBWEBVIEW_H
