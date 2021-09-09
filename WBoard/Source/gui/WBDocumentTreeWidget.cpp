#include "WBDocumentTreeWidget.h"

#include "document/WBDocumentProxy.h"

#include "core/WBSettings.h"
#include "core/WBApplication.h"
#include "core/WBPersistenceManager.h"
#include "core/WBMimeData.h"
#include "core/WBApplicationController.h"
#include "core/WBDocumentManager.h"
#include "document/WBDocumentController.h"

#include "adaptors/WBThumbnailAdaptor.h"
#include "adaptors/WBSvgSubsetAdaptor.h"
#include "frameworks/WBFileSystemUtils.h"

#include "core/memcheck.h"

WBDocumentTreeWidget::WBDocumentTreeWidget(QWidget * parent)
    : QTreeWidget(parent)
    , mSelectedProxyTi(0)
    , mDropTargetProxyTi(0)
{
    setDragDropMode(QAbstractItemView::InternalMove);
    setAutoScroll(true);

    mScrollTimer = new QTimer(this);
    connect(WBDocumentManager::documentManager(), SIGNAL(documentUpdated(WBDocumentProxy*))
            , this, SLOT(documentUpdated(WBDocumentProxy*)));

    connect(this, SIGNAL(itemChanged(QTreeWidgetItem *, int))
            , this,  SLOT(itemChangedValidation(QTreeWidgetItem *, int)));
    connect(mScrollTimer, SIGNAL(timeout())
            , this, SLOT(autoScroll()));
}


WBDocumentTreeWidget::~WBDocumentTreeWidget()
{
    // NOOP
}

void WBDocumentTreeWidget::itemChangedValidation(QTreeWidgetItem * item, int column)
{

    if (column == 0)
    {
        WBDocumentGroupTreeItem* group = dynamic_cast<WBDocumentGroupTreeItem *>(item);
        if(group)
        {
            QString name = group->text(0);

            for(int i = 0; i < topLevelItemCount (); i++)
            {
                QTreeWidgetItem *someTopLevelItem = topLevelItem(i);

                if (someTopLevelItem != group &&
                        someTopLevelItem->text(0) == name)
                {
                    group->setText(0, tr("%1 (copy)").arg(name));
                }
            }
        }
    }
}

Qt::DropActions WBDocumentTreeWidget::supportedDropActions() const
{
    return Qt::MoveAction | Qt::CopyAction;
}


void WBDocumentTreeWidget::mousePressEvent(QMouseEvent *event)
{
    QTreeWidgetItem* twItem = this->itemAt(event->pos());

    mSelectedProxyTi = dynamic_cast<WBDocumentProxyTreeItem*>(twItem);

    QTreeWidget::mousePressEvent(event);
}


void WBDocumentTreeWidget::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}


void WBDocumentTreeWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event);

    if (mScrollTimer->isActive())
    {
        mScrollMagnitude = 0;
        mScrollTimer->stop();
    }

    if (mDropTargetProxyTi)
    {
        mDropTargetProxyTi->setBackground(0, mBackground);
        mDropTargetProxyTi = 0;
    }
}


void WBDocumentTreeWidget::dragMoveEvent(QDragMoveEvent *event)
{
    QRect boundingFrame = frameRect();
    //setting up automatic scrolling
    const int SCROLL_DISTANCE = 4;
    int bottomDist = boundingFrame.bottom() - event->pos().y(), topDist = boundingFrame.top() - event->pos().y();
    if(qAbs(bottomDist) <= SCROLL_DISTANCE)
    {
        mScrollMagnitude = (SCROLL_DISTANCE - bottomDist)*4;
        if(verticalScrollBar()->isVisible() && !mScrollTimer->isActive()) mScrollTimer->start(100);
    }
    else if(qAbs(topDist) <= SCROLL_DISTANCE)
    {
        mScrollMagnitude = (- SCROLL_DISTANCE - topDist)*4;
        if(verticalScrollBar()->isVisible() && !mScrollTimer->isActive()) mScrollTimer->start(100);
    }
    else
    {
        mScrollMagnitude = 0;
        mScrollTimer->stop();
    }


    QTreeWidgetItem* underlyingItem = this->itemAt(event->pos());

    if (event->mimeData()->hasFormat(WBApplication::mimeTypeUniboardPage))
    {
        WBDocumentProxyTreeItem *targetProxyTreeItem = dynamic_cast<WBDocumentProxyTreeItem*>(underlyingItem);
        if (targetProxyTreeItem && targetProxyTreeItem != mSelectedProxyTi)
        {
            event->setDropAction(Qt::CopyAction);
            event->accept();
        }
        else
        {
            event->ignore();
        }
    }
    else
    {
        WBDocumentGroupTreeItem *groupItem = dynamic_cast<WBDocumentGroupTreeItem*>(underlyingItem);

        if (groupItem && mSelectedProxyTi && groupItem != mSelectedProxyTi->parent())
            event->acceptProposedAction();
        else
            event->ignore();
    }

    if (event->isAccepted())
    {
        if (mDropTargetProxyTi)
        {
            if (underlyingItem != mDropTargetProxyTi)
            {
                mBackground = underlyingItem->background(0);
                mDropTargetProxyTi->setBackground(0, mBackground);
                mDropTargetProxyTi = underlyingItem;
                mDropTargetProxyTi->setBackground(0, QBrush(QColor("#6682b5")));
            }
        }
        else
        {
            mBackground = underlyingItem->background(0);
            mDropTargetProxyTi = underlyingItem;
            mDropTargetProxyTi->setBackground(0, QBrush(QColor("#6682b5")));
        }
    }
    else if (mDropTargetProxyTi)
    {
        mDropTargetProxyTi->setBackground(0, mBackground);
        mDropTargetProxyTi = 0;
    }
}


void WBDocumentTreeWidget::focusInEvent(QFocusEvent *event)
{
    QTreeWidget::focusInEvent(event);
}

void WBDocumentTreeWidget::dropEvent(QDropEvent *event)
{
    if (mDropTargetProxyTi) {
        mDropTargetProxyTi->setBackground(0, mBackground);
        mDropTargetProxyTi = 0;
    }

    QTreeWidgetItem * underlyingItem = this->itemAt(event->pos());

    // If the destination is a folder, move the selected document(s) there
    WBDocumentGroupTreeItem * destinationFolder = dynamic_cast<WBDocumentGroupTreeItem*>(underlyingItem);

    if (destinationFolder) {
        WBDocumentProxyTreeItem * lastMovedDocument;
        foreach(QTreeWidgetItem * item, this->selectedItems()) {
            WBDocumentProxyTreeItem * document = dynamic_cast<WBDocumentProxyTreeItem*>(item);
            if (document && moveDocument(document, destinationFolder))
                lastMovedDocument = document;
        }

        if (lastMovedDocument) {
            expandItem(destinationFolder);
            scrollToItem(lastMovedDocument);
            setCurrentItem(lastMovedDocument);
            lastMovedDocument->setSelected(true);

            event->setDropAction(Qt::IgnoreAction);
            event->accept();
        }
    }

    // If the destination is a document and the dropped item is a page, copy the page to that document
    else {
        QTreeWidgetItem* underlyingTreeItem = this->itemAt(event->pos());

        WBDocumentProxyTreeItem *targetProxyTreeItem = dynamic_cast<WBDocumentProxyTreeItem*>(underlyingTreeItem);
        if (targetProxyTreeItem && targetProxyTreeItem != mSelectedProxyTi)
        {
            if (event->mimeData()->hasFormat(WBApplication::mimeTypeUniboardPage))
            {
                event->setDropAction(Qt::CopyAction);
                event->accept();

                const WBMimeData *mimeData = qobject_cast <const WBMimeData*>(event->mimeData());

                if (mimeData && mimeData->items().size() > 0)
                {
                        int count = 0;
                        int total = mimeData->items().size();

                        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

                    foreach (WBMimeDataItem sourceItem, mimeData->items())
                    {
                        count++;

                        WBApplication::applicationController->showMessage(tr("Copying page %1/%2").arg(count).arg(total), true);

                        WBGraphicsScene *scene = WBPersistenceManager::persistenceManager()->loadDocumentScene(sourceItem.documentProxy(), sourceItem.sceneIndex());
                        if (scene)
                        {
                            WBGraphicsScene* sceneClone = scene->sceneDeepCopy();

                            WBDocumentProxy *targetDocProxy = targetProxyTreeItem->proxy();

                            foreach (QUrl relativeFile, scene->relativeDependencies())
                            {
                                QString source = scene->document()->persistencePath() + "/" + relativeFile.toString();
                                QString target = targetDocProxy->persistencePath() + "/" + relativeFile.toString();

                                QString sourceDecoded = scene->document()->persistencePath() + "/" + relativeFile.toString(QUrl::DecodeReserved);
                                QString targetDecoded = targetDocProxy->persistencePath() + "/" + relativeFile.toString(QUrl::DecodeReserved);

                                if(QFileInfo(source).isDir())
                                    WBFileSystemUtils::copyDir(source,target);
                                else{
                                    QFileInfo fi(targetDecoded);
                                    QDir d = fi.dir();
                                    d.mkpath(d.absolutePath());
                                    QFile::copy(sourceDecoded, targetDecoded);
                                }
                            }

                            WBPersistenceManager::persistenceManager()->insertDocumentSceneAt(targetDocProxy, sceneClone, targetDocProxy->pageCount());

                            //due to incorrect generation of thumbnails of invisible scene I've used direct copying of thumbnail files
                            //it's not universal and good way but it's faster
                            QString from = sourceItem.documentProxy()->persistencePath() + WBFileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", sourceItem.sceneIndex());
                            QString to  = targetDocProxy->persistencePath() + WBFileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", targetDocProxy->pageCount());
                            QFile::remove(to);
                            QFile::copy(from, to);
                          }
                    }

                    QApplication::restoreOverrideCursor();

                    WBApplication::applicationController->showMessage(tr("%1 pages copied", "", total).arg(total), false);
                }
            }
            else
            {
                event->setDropAction(Qt::IgnoreAction);
                event->ignore();
            }
        }
    }
}


void WBDocumentTreeWidget::documentUpdated(WBDocumentProxy *pDocument)
{
	WBDocumentProxyTreeItem *treeItem = WBApplication::documentController->findDocument(pDocument);//zhusizhi findDocument
    if (treeItem)
    {
        QTreeWidgetItem * parent = treeItem->parent();

        if (parent)
        {
            for (int i = 0; i < parent->indexOfChild(treeItem); i++)
            {
                QTreeWidgetItem *ti = parent->child(i);
                WBDocumentProxyTreeItem* pi = dynamic_cast<WBDocumentProxyTreeItem*>(ti);
                if (pi)
                {
                    if (pDocument->metaData(WBSettings::documentDate).toString() >= pi->proxy()->metaData(WBSettings::documentDate).toString())
                    {
                        bool selected = treeItem->isSelected();
                        parent->removeChild(treeItem);
                        parent->insertChild(i, treeItem);
                        for (int j = 0; j < selectedItems().count(); j++)
                            selectedItems().at(j)->setSelected(false);
                        if (selected)
                            treeItem->setSelected(true);
                        break;
                    }
                }
            }
        }
    }
}

/**
 * @brief Move a document to the specified destination folder
 * @param document Pointer to the document to move
 * @param destinationFolder Pointer to the folder to move the document to
 * @return true if document was moved successfully, false otherwise
 */
bool WBDocumentTreeWidget::moveDocument(WBDocumentProxyTreeItem* document, WBDocumentGroupTreeItem* destinationFolder)
{
    if (!document || !(document->proxy()) || !destinationFolder)
        return false;

    WBDocumentGroupTreeItem * sourceFolder = dynamic_cast<WBDocumentGroupTreeItem*>(document->parent());
    bool documentIsInTrash = (sourceFolder && sourceFolder->isTrashFolder());

    if (documentIsInTrash && destinationFolder->isTrashFolder())
        return false;

    if (!documentIsInTrash && document->proxy()->groupName() == destinationFolder->groupName())
        return false;

    QString destinationFolderName;

    if (destinationFolder->isTrashFolder()) {
       // WBApplication::app()->documentController->moveDocumentToTrash(sourceFolder, document);//zhusizhi
        destinationFolderName = document->proxy()->metaData(WBSettings::documentGroupName).toString();
    }

    else {
        if (destinationFolder->groupName() == WBApplication::app()->documentController->defaultDocumentGroupName())
            destinationFolderName = "";
        else
            destinationFolderName = destinationFolder->groupName();
    }

    // Update the folder name in the document
    document->proxy()->setMetaData(WBSettings::documentGroupName, destinationFolderName);
    WBPersistenceManager::persistenceManager()->persistDocumentMetadata(document->proxy());

    // Remove document from its old folder
    document->parent()->removeChild(document);

    // Insert document at the right spot in the destination folder (ordered by document date)
    int i = 0;
    for (i = 0; i < destinationFolder->childCount(); i++) {
        QTreeWidgetItem *ti = destinationFolder->child(i);
        WBDocumentProxyTreeItem* pi = dynamic_cast<WBDocumentProxyTreeItem*>(ti);
        if (pi && document->proxy()->metaData(WBSettings::documentDate).toString() >= pi->proxy()->metaData(WBSettings::documentDate).toString())
            break;
    }

    destinationFolder->insertChild(i, document);

    // Update editable status of the document if it was moved to or from the trash
    if (documentIsInTrash)
        document->setFlags(document->flags() | Qt::ItemIsEditable);

    if (destinationFolder->isTrashFolder())
        document->setFlags(document->flags() ^ Qt::ItemIsEditable);

    return true;
}

WBDocumentProxyTreeItem::WBDocumentProxyTreeItem(QTreeWidgetItem * parent, WBDocumentProxy* proxy, bool isEditable)
    : QTreeWidgetItem()
    , mProxy(proxy)
{
    Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled;

    if (isEditable)
        flags |= Qt::ItemIsEditable;

    setFlags(flags);

    int i = 0;
    for (i = 0; i < parent->childCount(); i++)
    {
        QTreeWidgetItem *ti = parent->child(i);
        WBDocumentProxyTreeItem* pi = dynamic_cast<WBDocumentProxyTreeItem*>(ti);
        if (pi)
        {
            if (proxy->metaData(WBSettings::documentDate).toString() >= pi->proxy()->metaData(WBSettings::documentDate).toString())
            {
                break;
            }
        }
    }
    parent->insertChild(i, this);
}

bool WBDocumentProxyTreeItem::isInTrash()
{
    WBDocumentGroupTreeItem * parentFolder = dynamic_cast<WBDocumentGroupTreeItem*>(this->parent());
    if (parentFolder)
        return parentFolder->isTrashFolder();
    else {
        qWarning() << "WBDocumentProxyTreeItem::isInTrash: document has no parent folder. Assuming it is in trash.";
        return true;
    }
}


WBDocumentGroupTreeItem::WBDocumentGroupTreeItem(QTreeWidgetItem *parent, bool isEditable)
    : QTreeWidgetItem(parent)
{
    Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;
    if (isEditable)
        flags |= Qt::ItemIsEditable;
    setFlags(flags);
}


WBDocumentGroupTreeItem::~WBDocumentGroupTreeItem()
{
    // NOOP
}


void WBDocumentGroupTreeItem::setGroupName(const QString& groupName)
{
    setText(0, groupName);
}


QString WBDocumentGroupTreeItem::groupName() const
{
    return text(0);
}

bool WBDocumentGroupTreeItem::isTrashFolder() const
{
    return (0 == (flags() & Qt::ItemIsEditable)) &&  WBApplication::app()->documentController && (groupName() == WBApplication::app()->documentController->documentTrashGroupName());
}

bool WBDocumentGroupTreeItem::isDefaultFolder() const
{
    return (0 == (flags() & Qt::ItemIsEditable)) && WBApplication::app()->documentController && (groupName() == WBApplication::app()->documentController->defaultDocumentGroupName());
}


void WBDocumentTreeWidget::autoScroll()
{
    this->verticalScrollBar()->setValue(this->verticalScrollBar()->value() + mScrollMagnitude);
}
