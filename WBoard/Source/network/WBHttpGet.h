#ifndef WBHTTPGET_H_
#define WBHTTPGET_H_

#include <QtCore>
#include <QtNetwork>
#include <QDropEvent>

class WBHttpGet : public QObject
{
    Q_OBJECT

public:
    WBHttpGet(QObject* parent = 0);
    virtual ~WBHttpGet();

    QNetworkReply* get(QUrl pUrl, QPointF pPoint = QPointF(0, 0), QSize pSize = QSize(0, 0), bool isBackground = false);
//        QNetworkReply* get(const sDownloadFileDesc &downlinfo);

signals:
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished(bool pSuccess, QUrl sourceUrl, QString pContentTypeHeader
            , QByteArray pData, QPointF pPos, QSize pSize, bool isBackground);
//        void downloadFinished(bool pSuccess, QUrl sourceUrl, QString pContentTypeHeader, QByteArray pData
//                              , sDownloadFileDesc downlInfo);

private slots:
    void readyRead();
    void requestFinished();
    void downloadProgressed(qint64 bytesReceived, qint64 bytesTotal);

private:
    QByteArray mDownloadedBytes;
    QNetworkReply* mReply;
    QPointF mPos;
    QSize mSize;

    bool mIsBackground;
    int mRequestID;
    int mRedirectionCount;
    bool mIsSelfAborting;
//        sDownloadFileDesc mDownloadInfo;
};

#endif /* WBHTTPGET_H_ */

