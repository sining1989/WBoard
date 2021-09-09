#ifndef WBDOWNLOADWIDGET_H
#define WBDOWNLOADWIDGET_H

#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QItemDelegate>

#include "core/WBDownloadManager.h"

typedef enum{
    eItemColumn_Desc,
    eItemColumn_Close
}eItemColumn;

class WBDownloadProgressDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    WBDownloadProgressDelegate(QObject* parent=0);

protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class WBDownloadWidget : public QWidget
{
    Q_OBJECT
public:
    WBDownloadWidget(QWidget* parent=0, const char* name="WBDownloadWidget");
    ~WBDownloadWidget();

private slots:
    void onFileAddedToDownload();
    void onDownloadUpdated(int id, qint64 crnt, qint64 total);
    void onDownloadFinished(int id);
    void onCancelClicked();
    void onItemClicked(QTreeWidgetItem* pItem, int col);

private:
    void addCurrentDownloads();
    void addPendingDownloads();

    QVBoxLayout* mpLayout;
    QHBoxLayout* mpBttnLayout;
    QTreeWidget* mpTree;
    QPushButton* mpCancelBttn;
    QTreeWidgetItem* mpItem;
    WBDownloadProgressDelegate mProgressBarDelegate;
};

#endif // WBDOWNLOADWIDGET_H
