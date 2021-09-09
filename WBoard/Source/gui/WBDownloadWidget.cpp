#include <QDebug>
#include <QHeaderView>
#include <QStyleOptionProgressBarV2>
#include <QApplication>

#include "WBDownloadWidget.h"

#include "globals/WBGlobals.h"

#include "core/WBApplication.h"
#include "core/memcheck.h"

/**
 * \brief Constructor
 * @param parent as the parent widget
 * @param name as the widget object name
 */
WBDownloadWidget::WBDownloadWidget(QWidget *parent, const char *name):QWidget(parent)
  , mpLayout(NULL)
  , mpBttnLayout(NULL)
  , mpTree(NULL)
  , mpCancelBttn(NULL)
  , mpItem(NULL)
{
    setObjectName(name);
    setWindowTitle(tr("Downloading files"));
    SET_STYLE_SHEET();
    resize(400, 300);

    mpLayout = new QVBoxLayout(this);
    setLayout(mpLayout);

    mpTree = new QTreeWidget(this);
    mpTree->setRootIsDecorated(false);
    mpTree->setColumnCount(2);
    mpTree->header()->setStretchLastSection(false);
    mpTree->header()->setSectionResizeMode(eItemColumn_Desc, QHeaderView::Stretch);
    mpTree->header()->setSectionResizeMode(eItemColumn_Close, QHeaderView::Custom);
    mpTree->resizeColumnToContents(eItemColumn_Close);
    mpTree->header()->close();
    mpLayout->addWidget(mpTree, 1);

    mpBttnLayout = new QHBoxLayout();
    mpBttnLayout->addStretch(1);
    mpCancelBttn = new QPushButton(tr("Cancel"), this);
    mpCancelBttn->setObjectName("DockPaletteWidgetButton");
    mpBttnLayout->addWidget(mpCancelBttn, 0);
    mpLayout->addLayout(mpBttnLayout);

    connect(WBDownloadManager::downloadManager(), SIGNAL(fileAddedToDownload()), this, SLOT(onFileAddedToDownload()));
    connect(WBDownloadManager::downloadManager(), SIGNAL(downloadUpdated(int,qint64,qint64)), this, SLOT(onDownloadUpdated(int,qint64,qint64)));
    connect(WBDownloadManager::downloadManager(), SIGNAL(downloadFinished(int)), this, SLOT(onDownloadFinished(int)));
    connect(mpCancelBttn, SIGNAL(clicked()), this, SLOT(onCancelClicked()));
    connect(mpTree, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(onItemClicked(QTreeWidgetItem*,int)));
}

/**
 * \brief Destructor
 */
WBDownloadWidget::~WBDownloadWidget()
{
    if(NULL != mpCancelBttn)
    {
        delete mpCancelBttn;
        mpCancelBttn = NULL;
    }
    if(NULL != mpTree)
    {
        delete mpTree;
        mpTree = NULL;
    }
    if(NULL != mpBttnLayout)
    {
        delete mpBttnLayout;
        mpBttnLayout = NULL;
    }
    if(NULL != mpLayout)
    {
        delete mpLayout;
        mpLayout = NULL;
    }
}

/**
 * \brief Refresh the tree of downloaded files
 */
void WBDownloadWidget::onFileAddedToDownload()
{
    if(NULL != mpTree)
    {
        mpTree->clear();
        addCurrentDownloads();
        addPendingDownloads();
    }
}

/**
 * \brief Add the current downloads
 */
void WBDownloadWidget::addCurrentDownloads()
{
    QVector<sDownloadFileDesc> actualDL = WBDownloadManager::downloadManager()->currentDownloads();
    qDebug() << "Actual downloads size: " << actualDL.size();
    for(int i=0; i<actualDL.size();i++)
    {
        mpItem = new QTreeWidgetItem(mpTree);
        mpItem->setText(eItemColumn_Desc, actualDL.at(i).name);
        mpItem->setData(eItemColumn_Desc, Qt::UserRole, QVariant(actualDL.at(i).id));
        mpItem->setIcon(eItemColumn_Close, QIcon(":images/close.svg"));
        mpTree->addTopLevelItem(mpItem);
        mpItem = new QTreeWidgetItem(mpTree);
        mpItem->setData(eItemColumn_Desc, Qt::UserRole, actualDL.at(i).currentSize);
        mpItem->setData(eItemColumn_Desc, Qt::UserRole + 1, actualDL.at(i).totalSize);
        mpItem->setData(eItemColumn_Desc, Qt::UserRole + 2, actualDL.at(i).id);
        mpTree->addTopLevelItem(mpItem);
        mpTree->setItemDelegateForRow(((i+1)*2)-1, &mProgressBarDelegate);
    }
}

/**
 * \brief Add the pending downloads
 */
void WBDownloadWidget::addPendingDownloads()
{
    QVector<sDownloadFileDesc> pendingDL = WBDownloadManager::downloadManager()->pendingDownloads();
    for(int i=0; i<pendingDL.size(); i++)
    {
        mpItem = new QTreeWidgetItem(mpTree);
        mpItem->setText(eItemColumn_Desc, pendingDL.at(i).name);
        mpItem->setData(eItemColumn_Desc, Qt::UserRole, QVariant(pendingDL.at(i).id));
        mpTree->addTopLevelItem(mpItem);
    }
}

/**
 * \brief Update the progress bar
 * @param id as the downloaded file id
 * @param crnt as the current transfered size
 * @param total as the total size of the file
 */
void WBDownloadWidget::onDownloadUpdated(int id, qint64 crnt, qint64 total)
{
    if(NULL != mpTree)
    {
        QAbstractItemModel* model = mpTree->model();
        if(NULL != model)
        {
            for(int i=0; i< model->rowCount(); i++)
            {
                QModelIndex currentIndex = model->index(i, eItemColumn_Desc);
                if(id == currentIndex.data(Qt::UserRole + 2))
                {
                    // We found the right item, now we update the progress bar
                    model->setData(currentIndex, crnt, Qt::UserRole);
                    model->setData(currentIndex, total, Qt::UserRole + 1);
                    break;
                }
            }
        }
    }
}

/**
 * \brief Handles the download finish notification
 * @param id as the downloaded file id
 */
void WBDownloadWidget::onDownloadFinished(int id)
{
    Q_UNUSED(id);
    // Refresh the file's list
    onFileAddedToDownload();
}

/**
 * \brief Handles the Cancel button action
 */
void WBDownloadWidget::onCancelClicked()
{
    WBDownloadManager::downloadManager()->cancelDownloads();
}

/**
 * \brief Handles the item click notification
 * @param pItem as the item clicked
 * @param col as the column containing the item clicked
 */
void WBDownloadWidget::onItemClicked(QTreeWidgetItem *pItem, int col)
{
    if( eItemColumn_Close == col
            && "" != pItem->text(eItemColumn_Desc)){

        // Stop the download of the clicked item and remove it from the list
        WBDownloadManager::downloadManager()->cancelDownload(pItem->data(eItemColumn_Desc, Qt::UserRole).toInt());
    }
}

// ---------------------------------------------------------------------------------------------
WBDownloadProgressDelegate::WBDownloadProgressDelegate(QObject *parent):QItemDelegate(parent)
{

}

void WBDownloadProgressDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionProgressBarV2 opt;
    opt.rect = option.rect;
    opt.minimum = 0;
    opt.maximum = index.data(Qt::UserRole + 1).toInt();
    opt.progress = index.data(Qt::UserRole).toInt();

    if(0 == index.column()){
        QApplication::style()->drawControl(QStyle::CE_ProgressBar, &opt, painter, 0);
    }
}
