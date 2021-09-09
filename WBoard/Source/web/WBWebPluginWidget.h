#ifndef WBWEBPLUGINWIDGET_H
#define WBWEBPLUGINWIDGET_H

#include <QWidget>
#include <QProgressBar>
#include <QToolButton>

#include "network/WBHttpGet.h"

class WBWebPluginWidget : public QWidget
{
    Q_OBJECT

public:
    WBWebPluginWidget(const QUrl &url, QWidget *parent = 0);
    virtual ~WBWebPluginWidget();

    virtual QString title() const;

protected:
    virtual void handleFile(const QString &filePath) = 0;

    virtual void paintEvent(QPaintEvent *paintEvent) = 0;
    virtual void resizeEvent(QResizeEvent *event);

private slots:
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished(bool pSuccess, QUrl sourceUrl, QString pContentTypeHeader, QByteArray pData);

private:
    QProgressBar mLoadingProgressBar;
};

#endif // WBWEBPLUGINWIDGET_H
