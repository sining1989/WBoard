#include <QtWidgets>

#include "WBWebPluginWidget.h"
//#include "pdf/UBWebPluginPDFWidget.h"
#include "frameworks/WBFileSystemUtils.h"

#include "core/memcheck.h"

WBWebPluginWidget::WBWebPluginWidget(const QUrl &url, QWidget *parent)
    : QWidget(parent)
    , mLoadingProgressBar(this)
{
    WBHttpGet* httpGet = new WBHttpGet(this);
    
    connect(httpGet, SIGNAL(downloadFinished(bool, QUrl, QString, QByteArray, QPointF, QSize, bool)), this, SLOT(downloadFinished(bool, QUrl, QString, QByteArray)));
    connect(httpGet, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(downloadProgress(qint64, qint64)));
    
    httpGet->get(url);
}

WBWebPluginWidget::~WBWebPluginWidget()
{
    // NOOP
}

void WBWebPluginWidget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);

    mLoadingProgressBar.move(geometry().center() - mLoadingProgressBar.geometry().center());
}

void WBWebPluginWidget::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal == -1)
    {
        mLoadingProgressBar.setMinimum(0);
        mLoadingProgressBar.setMaximum(0);
    } 
    else
    {
        mLoadingProgressBar.setMaximum(bytesTotal);
        mLoadingProgressBar.setValue(bytesReceived);
    }
}

void WBWebPluginWidget::downloadFinished(bool pSuccess, QUrl sourceUrl, QString pContentTypeHeader, QByteArray pData)
{
    Q_UNUSED(pSuccess);
    Q_UNUSED(pContentTypeHeader);

    QString tempFile = WBFileSystemUtils::createTempDir("UBWebPluginTemplate") + "/" + QFileInfo(sourceUrl.path()).fileName();
    QFile pdfFile(tempFile);
    pdfFile.open(QIODevice::WriteOnly);
    pdfFile.write(pData);
    pdfFile.close();
    handleFile(tempFile);
    mLoadingProgressBar.hide();
    update();
}

QString WBWebPluginWidget::title() const
{
    return QString(tr("Loading..."));
}
