#ifndef WBDOWNLOADTHREAD_H
#define WBDOWNLOADTHREAD_H

#include <QThread>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

class WBDownloadThread : public QThread
{
    Q_OBJECT
public:
    WBDownloadThread(QObject* parent=0, const char* name="WBDownloadThread");
    ~WBDownloadThread();
    void stopDownload();
    void startDownload(int id, QString url);

signals:
    void downloadFinised(int id);
    void downloadProgress(int id, qint64 current, qint64 total);

protected:
    virtual void run();

private slots:
    void onDownloadProgress(qint64 received, qint64 total);
    void onDownloadFinished();

private:
    bool mbRun;
    int mID;
    QString mUrl;
    QNetworkAccessManager* mpNam;
    QNetworkReply* mpReply;
};

#endif // WBDOWNLOADTHREAD_H
