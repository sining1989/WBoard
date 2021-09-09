#ifndef WBDOWNLOADMANAGER_H_
#define WBDOWNLOADMANAGER_H_

#include "ui_downloads.h"
#include "ui_downloaditem.h"

#include <QtNetwork>
#include <QtCore>
#include <QFileIconProvider>

class WBDownloadItem : public QWidget, public Ui_DownloadItem
{
    Q_OBJECT	  

public:
    WBDownloadItem(QNetworkReply *reply = 0, bool requestFileName = false, QWidget *parent = 0, QString customDownloadPath = QString());
    bool downloading() const;
    bool downloadedSuccessfully() const;

    QUrl m_url;

    QFile m_output;
    QNetworkReply *m_reply;
signals:
void statusChanged();

private slots:
    void stop();
    void tryAgain();
    void open();

    void downloadReadyRead();
    void error(QNetworkReply::NetworkError code);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void metaDataChanged();
    void finished();

private:
    void getFileName();
    void init();
    void updateInfoLabel();
    QString dataString(int size) const;

    QString saveFileName(const QString &directory) const;

    bool mRequestFileName;
    qint64 mBytesReceived;
    QTime mDownloadTime;
    QString mCustomDownloadPath;

};

class WBAutoSaver;
class WBDownloadModel;

class WBDownloadManager_ : public QDialog, public Ui_DownloadDialog
{
    Q_OBJECT
    Q_PROPERTY(RemovePolicy removePolicy READ removePolicy WRITE setRemovePolicy)
    Q_ENUMS(RemovePolicy)

public:
    enum RemovePolicy {
        Never,
        Exit,
        SuccessFullDownload
    };

    WBDownloadManager_(QWidget *parent = 0);
    ~WBDownloadManager_();
    int activeDownloads() const;

    RemovePolicy removePolicy() const;
    void setRemovePolicy(RemovePolicy policy);

public slots:
    void download(const QNetworkRequest &request, bool requestFileName = false);
    inline void download(const QUrl &url, bool requestFileName = false)
        { download(QNetworkRequest(url), requestFileName); }
    void handleUnsupportedContent(QNetworkReply *reply, bool requestFileName = false, QString customDownloadPath = QString());
    void cleanup();

private slots:
    void save() const;
    void updateRow();
    void processDownloadedContent(QNetworkReply *reply, bool requestFileName, QString customDownloadPath = QString());

private:
    void addItem(WBDownloadItem *item);
    void updateItemCount();
    void load();

    WBAutoSaver *mAutoSaver;
    WBDownloadModel *mModel;
    QNetworkAccessManager *mManager;
    QFileIconProvider *mIconProvider;
    QList<WBDownloadItem*> mDownloads;
    RemovePolicy m_RemovePolicy;
    friend class WBDownloadModel;

    bool mIsLoading;
};

class WBDownloadModel : public QAbstractListModel
{
    friend class WBDownloadManager_;
    Q_OBJECT

public:
    WBDownloadModel(WBDownloadManager_ *downloadManager, QObject *parent = 0);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());

private:
    WBDownloadManager_ *mDownloadManager;

};

#endif // WBDOWNLOADMANAGER_H_

