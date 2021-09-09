#include "WBDocumentController.h"

#include <QtCore>
#include <QtWidgets>

#include "frameworks/WBFileSystemUtils.h"
#include "frameworks/WBStringUtils.h"
#include "frameworks/WBPlatformUtils.h"

#include "core/WBApplication.h"
#include "core/WBPersistenceManager.h"
#include "core/WBDocumentManager.h"
#include "core/WBApplicationController.h"
#include "core/WBSettings.h"
#include "core/WBSetting.h"
#include "core/WBMimeData.h"
#include "core/WBForeignObjectsHandler.h"

#include "adaptors/WBExportPDF.h"
#include "adaptors/WBThumbnailAdaptor.h"

#include "adaptors/WBMetadataDcSubsetAdaptor.h"

#include "board/WBBoardController.h"
#include "board/WBBoardPaletteManager.h"
#include "board/WBDrawingController.h"


#include "gui/WBThumbnailView.h"
#include "gui/WBMousePressFilter.h"
#include "gui/WBMessageWindow.h"
#include "gui/WBMainWindow.h"
#include "gui/WBDocumentToolsPalette.h"

#include "domain/WBGraphicsScene.h"
#include "domain/WBGraphicsSvgItem.h"
#include "domain/WBGraphicsPixmapItem.h"

#include "document/WBDocumentProxy.h"

#include "ui_documents.h"
#include "ui_mainWindow.h"

#include "core/memcheck.h"

static bool lessThan(WBDocumentTreeNode *lValue, WBDocumentTreeNode *rValue)
{
    if (lValue->nodeType() == WBDocumentTreeNode::Catalog) {
        if (rValue->nodeType() == WBDocumentTreeNode::Catalog) {
            return lValue->nodeName() < rValue->nodeName();
        } else {
            return true;
        }
    } else {
        if (rValue->nodeType() == WBDocumentTreeNode::Catalog) {
            return false;
        } else {
            Q_ASSERT(lValue->proxyData());
            Q_ASSERT(rValue->proxyData());

            //return lValue->nodeName() < rValue->nodeName();

            QDateTime lTime = lValue->proxyData()->documentDate();
            QDateTime rTime = rValue->proxyData()->documentDate();

            return lTime > rTime;
        }
    }

    return false;
}


WBDocumentReplaceDialog::WBDocumentReplaceDialog(const QString &pIncommingName, const QStringList &pFileList, QWidget *parent, Qt::WindowFlags pFlags)
    : QDialog(parent, pFlags)
    , mFileNameList(pFileList)
    , mIncommingName(pIncommingName)
    , acceptText(tr("Accept"))
    , replaceText(tr("Replace"))
    , cancelText(tr("Cancel"))
    , mLabelText(0)
{
    this->setStyleSheet("background:white;");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QVBoxLayout *labelLayout = new QVBoxLayout();

    mLabelText = new QLabel(labelTextWithName(pIncommingName), this);
    mLineEdit = new QLineEdit(this);
    mLineEdit->setText(pIncommingName);
    mLineEdit->selectedText();

    labelLayout->addWidget(mLabelText);
    labelLayout->addWidget(mLineEdit);

    QHBoxLayout *buttonLayout = new QHBoxLayout();

    acceptButton = new QPushButton(acceptText, this);
    QPushButton *cancelButton = new QPushButton(cancelText, this);
    buttonLayout->addWidget(acceptButton);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(labelLayout);
    mainLayout->addLayout(buttonLayout);

    acceptButton->setEnabled(false);

    connect(acceptButton, SIGNAL(clicked()), this, SLOT(accept()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
    connect(mLineEdit, SIGNAL(textEdited(QString)), this, SLOT(reactOnTextChanged(QString)));

    reactOnTextChanged(mIncommingName);
}

void WBDocumentReplaceDialog::setRegexp(const QRegExp pRegExp)
{
    mValidator->setRegExp(pRegExp);
}
bool WBDocumentReplaceDialog::validString(const QString &pStr)
{
    Q_UNUSED(pStr);
    return mLineEdit->hasAcceptableInput();
}

void WBDocumentReplaceDialog::setFileNameAndList(const QString &fileName, const QStringList &pLst)
{
    mFileNameList = pLst;
    mIncommingName = fileName;
    mLabelText->setText(labelTextWithName(fileName));
    mLineEdit->setText(fileName);
    mLineEdit->selectAll();
    mLineEdit->selectedText();
}

QString WBDocumentReplaceDialog::labelTextWithName(const QString &documentName) const
{
    return tr("The name %1 is allready used.\nKeeping this name will replace the document.\nProviding a new name will create a new document.")
            .arg(documentName);
}

void WBDocumentReplaceDialog::accept()
{
    QDialog::accept();
}
void WBDocumentReplaceDialog::reject()
{
    mLineEdit->clear();
    emit closeDialog();

    QDialog::reject();
}

void WBDocumentReplaceDialog::reactOnTextChanged(const QString &pStr)
{
//     if !mFileNameList.contains(pStr.trimmed(), Qt::CaseSensitive)

    if (!validString(pStr)) {
        acceptButton->setEnabled(false);
        mLineEdit->setStyleSheet("background:#FFB3C8;");
        acceptButton->setEnabled(false);

    } else if (mFileNameList.contains(pStr.trimmed(), Qt::CaseSensitive)) {
        acceptButton->setEnabled(true);
        mLineEdit->setStyleSheet("background:#FFB3C8;");
        acceptButton->setText(replaceText);

    } else {
        acceptButton->setEnabled(true);
        mLineEdit->setStyleSheet("background:white;");
        acceptButton->setText(acceptText);
    }
}

WBDocumentTreeNode::WBDocumentTreeNode(Type pType, const QString &pName, const QString &pDisplayName, WBDocumentProxy *pProxy ) :
    mType(pType)
  , mName(pName)
  , mDisplayName(pDisplayName)
  , mProxy(pProxy)
{
    if (pDisplayName.isEmpty()) {
        mDisplayName = mName;
    }
    mParent = 0;
}

void WBDocumentTreeNode::addChild(WBDocumentTreeNode *pChild)
{
    if (pChild) {
        mChildren += pChild;
        pChild->mParent = this;
    }
}

void WBDocumentTreeNode::insertChild(int pIndex, WBDocumentTreeNode *pChild)
{
    if (pChild) {
        mChildren.insert(pIndex, pChild);
        pChild->mParent = this;
    }
}

void WBDocumentTreeNode::moveChild(WBDocumentTreeNode *child, int index, WBDocumentTreeNode *newParent)
{
    int childIndex = mChildren.indexOf(child);
    if (childIndex == -1) {
        return;
    }

    newParent->insertChild(index, child);
    mChildren.removeAt(childIndex);
}

void WBDocumentTreeNode::removeChild(int index)
{
    if (index < 0 || index > mChildren.count() - 1) {
        return;
    }

    WBDocumentTreeNode *curChild = mChildren[index];
    while (curChild->mChildren.count()) {
        curChild->removeChild(0);
    }

    mChildren.removeAt(index);
    delete curChild;
}

WBDocumentTreeNode *WBDocumentTreeNode::clone()
{
    return new WBDocumentTreeNode(this->mType
                                  , this->mName
                                  , this->mDisplayName
                                  , this->mProxy ? new WBDocumentProxy(*this->mProxy)
                                                 : 0);
}

QString WBDocumentTreeNode::dirPathInHierarchy()
{
    QString result;
    WBDocumentTreeNode *curNode = this;
    //protect the 2nd level items
    while (curNode->parentNode() && !curNode->isTopLevel()) {
        result.prepend(curNode->parentNode()->nodeName() + "/");
        curNode = curNode->parentNode();
    }

    if (result.endsWith("/")) {
        result.truncate(result.count() - 1);
    }

    return result;
}

WBDocumentTreeNode::~WBDocumentTreeNode()
{
    foreach (WBDocumentTreeNode *curChildren, mChildren) {
        delete(curChildren);
        curChildren = 0;
    }
    if (mProxy)
        delete mProxy;
}

bool WBDocumentTreeNode::findNode(WBDocumentTreeNode *node)
{
    WBDocumentTreeNode *parent = node->parentNode();

    bool hasFound = false;

    while(parent){
        if(parent == this){
            hasFound = true;
            break;
        }

        parent = parent->parentNode();
    }

    return hasFound;
}

WBDocumentTreeNode *WBDocumentTreeNode::nextSibling()
{
    WBDocumentTreeNode *parent = this->parentNode();
    WBDocumentTreeNode *nextSibling = NULL;

    int myIndex = parent->children().indexOf(this);
    int indexOfNextSibling = myIndex + 1;

    if(indexOfNextSibling < parent->children().size()){
        nextSibling = parent->children().at(indexOfNextSibling);
    }

    return nextSibling;
}

WBDocumentTreeNode *WBDocumentTreeNode::previousSibling()
{
    WBDocumentTreeNode *parent = this->parentNode();
    WBDocumentTreeNode *previousSibling = NULL;

    int myIndex = parent->children().indexOf(this);
    int indexOfPreviousSibling = myIndex - 1;

    if(indexOfPreviousSibling >= 0){
        previousSibling = parent->children().at(indexOfPreviousSibling);
    }

    return previousSibling;
}


WBDocumentTreeModel::WBDocumentTreeModel(QObject *parent) :
    QAbstractItemModel(parent)
  , mRootNode(0)
{
    WBDocumentTreeNode *rootNode = new WBDocumentTreeNode(WBDocumentTreeNode::Catalog, "root");

    QString trashName = WBSettings::trashedDocumentGroupNamePrefix;

    WBDocumentTreeNode *myDocsNode = new WBDocumentTreeNode(WBDocumentTreeNode::Catalog, WBPersistenceManager::myDocumentsName, tr("My documents"));
    rootNode->addChild(myDocsNode);
    //WBDocumentTreeNode *modelsNode = new WBDocumentTreeNode(WBDocumentTreeNode::Catalog, WBPersistenceManager::modelsName, tr("Models"));
    //rootNode->addChild(modelsNode);
    WBDocumentTreeNode *trashNode = new WBDocumentTreeNode(WBDocumentTreeNode::Catalog, trashName, tr("Trash"));
    rootNode->addChild(trashNode);
    //WBDocumentTreeNode *untitledDocumentsNode = new WBDocumentTreeNode(WBDocumentTreeNode::Catalog, WBPersistenceManager::untitledDocumentsName, tr("Untitled documents"));
    //myDocsNode->addChild(untitledDocumentsNode);

    setRootNode(rootNode);

    mRoot = index(0, 0, QModelIndex());
    mMyDocuments =  index(0, 0, QModelIndex());
    mTrash =  index(1, 0, QModelIndex());
    mUntitledDocuments = index(0, 0, mMyDocuments);
    mAscendingOrder = true;
}

QModelIndex WBDocumentTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!mRootNode || row < 0 || column < 0) {
        return QModelIndex();
    }

    WBDocumentTreeNode *nodeParent = nodeFromIndex(parent);
    if (!nodeParent || row > nodeParent->children().count() - 1) {
        return QModelIndex();
    }

    WBDocumentTreeNode *requiredNode = nodeParent->children().at(row);
    if(!requiredNode) {
        return QModelIndex();
    }

    QModelIndex resIndex = createIndex(row, column, requiredNode);

    return resIndex;
}

QModelIndex WBDocumentTreeModel::parent(const QModelIndex &child) const
{
    WBDocumentTreeNode *nodeChild = nodeFromIndex(child);
    if (!nodeChild) {
        return QModelIndex();
    }

    WBDocumentTreeNode *nodeParent = nodeChild->parentNode();
    if (!nodeParent) {
        return QModelIndex();
    }

    WBDocumentTreeNode *nodePreParent = nodeParent->parentNode();
    if (!nodePreParent) {
        return QModelIndex();
    }

    int row = nodePreParent->children().indexOf(nodeParent);

    QModelIndex resIndex = createIndex(row, 0, nodeParent);

    return resIndex;
}

int WBDocumentTreeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0) {
        return 0;
    }

    WBDocumentTreeNode *nodeParent = nodeFromIndex(parent);
    if (!nodeParent) {
        return 0;
    }

    return nodeParent->children().count();
}

int WBDocumentTreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return 3;
}

QVariant WBDocumentTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    WBDocumentTreeNode *dataNode = nodeFromIndex(index);

    if (!dataNode)
        return QVariant();


    if(role == Qt::DisplayRole){
        if(index.column() == 0){
            return dataNode->displayName();
        }else{
            WBDocumentProxy *proxy = proxyForIndex(index);

            QString displayText = "";

            if(proxy){
                QDateTime d;

                if(index.column() == 1){
                    d = proxy->documentDate();
                }else if(index.column() == 2){
                    d = proxy->lastUpdate();
                }

                displayText = d.toString("dd/MM/yyyy hh:mm");
            }

            return displayText;

        }
    }

    if(role == WBDocumentTreeModel::CreationDate){
        return findNodeDate(dataNode, WBSettings::documentDate);
    }

    if(role == WBDocumentTreeModel::UpdateDate){
        return findNodeDate(dataNode, WBSettings::documentUpdatedAt);
    }

    if(role == Qt::BackgroundRole){
        if (isConstant(index)) {
            return QBrush(0xD9DFEB);
        }

        if (mHighLighted.isValid() && index == mHighLighted) {
            return QBrush(0x6682B5);
        }
    }

    if(role == Qt::UserRole +1){
        return QVariant::fromValue(dataNode);
    }

    if (index.column() == 0) {
        switch (role) {
        case (Qt::DecorationRole) :
            if (mCurrentNode && mCurrentNode == dataNode) {
                return QIcon(":images/currentDocument.png");
            } else {
                if (index == trashIndex()) {
                    return QIcon(":images/trash.png");
                } else if (isConstant(index)) {
                    return QIcon(":images/libpalette/ApplicationsCategory.svg");
                }
                switch (static_cast<int>(dataNode->nodeType()))
                {
                    case WBDocumentTreeNode::Catalog :
                        return QIcon(":images/folder.png");
                    case WBDocumentTreeNode::Document :
                        return QIcon(":images/toolbar/board.png");
                }
            }
            break;
        case (Qt::FontRole) :
            if (isConstant(index)) {
                QFont font;
                font.setBold(true);
                return font;
            }
            break;
        case (Qt::ForegroundRole) :
            if (isConstant(index)) {
                return QColor(Qt::darkGray);
            }
            break;
        }
    }

    return QVariant();
}

bool WBDocumentTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    switch (role) {
    case Qt::EditRole:
        if (!index.isValid() || value.toString().isEmpty()) {
            return false;
        }
        setNewName(index, value.toString());
        return true;
    }
    return QAbstractItemModel::setData(index, value, role);
}

Qt::ItemFlags WBDocumentTreeModel::flags (const QModelIndex &index) const
{
    Qt::ItemFlags resultFlags = QAbstractItemModel::flags(index);
    WBDocumentTreeNode *indexNode = nodeFromIndex(index);

    if ( index.isValid() ) {
        if (!indexNode->isRoot() && !isConstant(index)) {
            if (!inTrash(index)) {
                resultFlags |= Qt::ItemIsEditable;
            }
            resultFlags |= Qt::ItemIsDragEnabled;
        }
        if (indexNode->nodeType() == WBDocumentTreeNode::Catalog) {
            resultFlags |= Qt::ItemIsDropEnabled;
        }
    }

    return resultFlags;
}

QDateTime WBDocumentTreeModel::findNodeDate(WBDocumentTreeNode *node, QString type) const
{
    if(type == WBSettings::documentDate){
        return findCatalogCreationDate(node);
    }else if(type == WBSettings::documentUpdatedAt){
        return findCatalogUpdatedDate(node);
    }

    return QDateTime();
}

QDateTime WBDocumentTreeModel::findCatalogUpdatedDate(WBDocumentTreeNode *node) const
{
    WBDocumentProxy *proxy = node->proxyData();

    if(proxy){
        return proxy->metaData(WBSettings::documentUpdatedAt).toDateTime();
    }else if(node->children().size() > 0){
        QDateTime d = findCatalogUpdatedDate(node->children().at(0));

        for(int i = 1; i < node->children().size(); i++){
            QDateTime dChild = findCatalogUpdatedDate(node->children().at(i));

            if(dChild != QDateTime()){
                if(mAscendingOrder){
                    d = qMin(d, dChild);
                }else{
                    d = qMax(d, dChild);
                }
            }

        }

        return d;
    }

    return QDateTime();
}

QDateTime WBDocumentTreeModel::findCatalogCreationDate(WBDocumentTreeNode *node) const
{
    WBDocumentProxy *proxy = node->proxyData();

    if(proxy){
        return proxy->metaData(WBSettings::documentDate).toDateTime();
    }else if(node->children().size() > 0){
        QDateTime d = findCatalogCreationDate(node->children().at(0));

        for(int i = 1; i < node->children().size(); i++){
            QDateTime dChild = findCatalogCreationDate(node->children().at(i));

            if(dChild != QDateTime()){
                if(mAscendingOrder){
                    d = qMin(d, dChild);
                }else{
                    d = qMax(d, dChild);
                }
            }

        }

        return d;
    }

    return QDateTime();
}

QStringList WBDocumentTreeModel::mimeTypes() const
{
    QStringList types;
    types << "text/uri-list" << "image/png" << "image/tiff" << "image/gif" << "image/jpeg";
    return types;
}

QMimeData *WBDocumentTreeModel::mimeData (const QModelIndexList &indexes) const
{
    WBDocumentTreeMimeData *mimeData = new WBDocumentTreeMimeData();
    QList <QModelIndex> indexList;
    QList<QUrl> urlList;

    foreach (QModelIndex index, indexes) {
        if (index.isValid()) {
            indexList.append(index);
            urlList.append(QUrl());
        }
    }

    mimeData->setUrls(urlList);
    mimeData->setIndexes(indexList);

    return mimeData;
}

bool WBDocumentTreeModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (action == Qt::IgnoreAction) {
        return false;
    }

    if (data->hasFormat(WBApplication::mimeTypeUniboardPage)) {
        WBDocumentTreeNode *curNode = nodeFromIndex(index(row - 1, column, parent));
        WBDocumentProxy *targetDocProxy = curNode->proxyData();
        const WBMimeData *ubMime = qobject_cast <const WBMimeData*>(data);
        if (!targetDocProxy || !ubMime || !ubMime->items().count()) {
            qDebug() << "an error ocured while parsing " << WBApplication::mimeTypeUniboardPage;
            return false;
        }

//        int count = 0;
        int total = ubMime->items().size();

        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

        foreach (WBMimeDataItem sourceItem, ubMime->items())
        {
            WBDocumentProxy *fromProxy = sourceItem.documentProxy();
            int fromIndex = sourceItem.sceneIndex();
            int toIndex = targetDocProxy->pageCount();

            WBPersistenceManager::persistenceManager()->copyDocumentScene(fromProxy, fromIndex,
                                                                          targetDocProxy, toIndex);
        }

        QApplication::restoreOverrideCursor();

        WBApplication::applicationController->showMessage(tr("%1 pages copied", "", total).arg(total), false);

        return true;
    }

    const WBDocumentTreeMimeData *mimeData = qobject_cast<const WBDocumentTreeMimeData*>(data);
    if (!mimeData) {
        qDebug() << "Incorrect mimeData, only internal one supported";
        return false;
    }

    if (!parent.isValid()) {
        return false;
    }

    WBDocumentTreeNode *newParentNode = nodeFromIndex(parent);

    if (!newParentNode) {
        qDebug() << "incorrect incoming parent node;";
        return false;
    }

    QList<QModelIndex> incomingIndexes = mimeData->indexes();

    foreach (QModelIndex curIndex, incomingIndexes)
    {
        if(curIndex.column() == 0){
            QModelIndex clonedTopLevel = copyIndexToNewParent(curIndex, parent, action == Qt::MoveAction ? aReference : aContentCopy);
            if (nodeFromIndex(curIndex) == mCurrentNode && action == Qt::MoveAction) {
                emit currentIndexMoved(clonedTopLevel, curIndex);
            }
        }
    }

    Q_UNUSED(action)
    Q_UNUSED(row)
    Q_UNUSED(column)
    Q_UNUSED(parent)

    return true;
}

bool WBDocumentTreeModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (row < 0 || row + count > rowCount(parent))
        return false;

    beginRemoveRows( parent, row, row + count - 1);

    WBDocumentTreeNode *parentNode = nodeFromIndex(parent);
    for (int i = row; i < row + count; i++) {
        WBDocumentTreeNode *curChildNode = parentNode->children().at(i);
        QModelIndex curChildIndex = parent.child(i, 0);
        if (curChildNode) {
            if (rowCount(curChildIndex)) {
                while (rowCount(curChildIndex)) {
                    removeRows(0, 1, curChildIndex);
                }
            }
        }
        mNewDocuments.removeAll(curChildNode->proxyData());
        parentNode->removeChild(i);

    }

    endRemoveRows();
    return true;
}

bool WBDocumentTreeModel::containsDocuments(const QModelIndex &index)
{
    for (int i = 0; i < rowCount(index); i++)
    {
        QModelIndex child = this->index(i, 0, index);
        if (isCatalog(child))
        {
            if (containsDocuments(child))
            {
                return true;
            }
        }
        else if (isDocument(child))
        {
           return true;
        }
        else
        {
            qDebug() << "Who the hell are you ?";
        }
    }

    return false;
}

QModelIndex WBDocumentTreeModel::indexForNode(WBDocumentTreeNode *pNode) const
{
    if (pNode == 0) {
        return QModelIndex();
    }

    return pIndexForNode(QModelIndex(), pNode);
}

QPersistentModelIndex WBDocumentTreeModel::persistentIndexForNode(WBDocumentTreeNode *pNode)
{
    return QPersistentModelIndex(indexForNode(pNode));
}

WBDocumentTreeNode *WBDocumentTreeModel::findProxy(WBDocumentProxy *pSearch, WBDocumentTreeNode *pParent) const
{
    foreach (WBDocumentTreeNode *curNode, pParent->children())
    {
        if (WBDocumentTreeNode::Catalog != curNode->nodeType())
        {
            if (curNode->proxyData()->theSameDocument(pSearch))
                return curNode;
        }
        else if (curNode->children().count())
        {
            WBDocumentTreeNode *recursiveDescendResult = findProxy(pSearch, curNode);
            if (recursiveDescendResult)
                return findProxy(pSearch, curNode);
        }
    }

    return 0;
}

QModelIndex WBDocumentTreeModel::pIndexForNode(const QModelIndex &parent, WBDocumentTreeNode *pNode) const
{
    for (int i = 0; i < rowCount(parent); i++) {
        QModelIndex curIndex = index(i, 0, parent);
        if (curIndex.internalPointer() == pNode) {
            return curIndex;
        } else if (rowCount(curIndex) > 0) {
            QModelIndex recursiveDescendIndex = pIndexForNode(curIndex, pNode);
            if (recursiveDescendIndex.isValid()) {
                return recursiveDescendIndex;
            }
        }
    }
    return QModelIndex();
}

void WBDocumentTreeModel::copyIndexToNewParent(const QModelIndexList &list, const QModelIndex &newParent, eCopyMode pMode)
{
    for(int i = 0; i < list.size(); i++){
        if(list.at(i).column() == 0){
            copyIndexToNewParent(list.at(i), newParent, pMode);
        }
    }
}

QPersistentModelIndex WBDocumentTreeModel::copyIndexToNewParent(const QModelIndex &source, const QModelIndex &newParent, eCopyMode pMode)
{
    WBDocumentTreeNode *nodeParent = nodeFromIndex(newParent);
    WBDocumentTreeNode *nodeSource = nodeFromIndex(source);

    if (!nodeParent || !nodeSource) {
        return QModelIndex();
    }

    //beginInsertRows(newParent, rowCount(newParent), rowCount(newParent));

    WBDocumentTreeNode *clonedNodeSource = 0;
    switch (static_cast<int>(pMode)) {
    case aReference:
        clonedNodeSource = nodeSource->clone();
        if (mNewDocuments.contains(nodeSource->proxyData())) { //update references for session documents
            mNewDocuments << clonedNodeSource->proxyData();

            WBPersistenceManager::persistenceManager()->reassignDocProxy(clonedNodeSource->proxyData(), nodeSource->proxyData());
        }
        break;

    case aContentCopy:
        WBDocumentProxy* duplicatedProxy = 0;
        if (nodeSource->nodeType() == WBDocumentTreeNode::Document && nodeSource->proxyData()) {
            duplicatedProxy = WBPersistenceManager::persistenceManager()->duplicateDocument(nodeSource->proxyData());
            duplicatedProxy->setMetaData(WBSettings::documentUpdatedAt, WBStringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
            WBMetadataDcSubsetAdaptor::persist(duplicatedProxy);
        }
        clonedNodeSource = new WBDocumentTreeNode(nodeSource->nodeType()
                                                  , nodeSource->nodeName()
                                                  , nodeSource->displayName()
                                                  , duplicatedProxy);
        break;
    }

    // Determine whether to provide a name with postfix if the name in current level already exists
    QString newName = clonedNodeSource->nodeName();
    if ((source.parent() != newParent
            || pMode != aReference)
            && (newParent != trashIndex() || !inTrash(newParent))) {
        newName = adjustNameForParentIndex(newName, newParent);
        clonedNodeSource->setNodeName(newName);
    }

    if (clonedNodeSource->proxyData()) {
        clonedNodeSource->proxyData()->setMetaData(WBSettings::documentGroupName, virtualPathForIndex(newParent));
        clonedNodeSource->proxyData()->setMetaData(WBSettings::documentName, newName);
        WBPersistenceManager::persistenceManager()->persistDocumentMetadata(clonedNodeSource->proxyData());
    }

    addNode(clonedNodeSource, newParent);
    //endInsertRows();

    QPersistentModelIndex newParentIndex = createIndex(rowCount(newParent), 0, clonedNodeSource);

    if (rowCount(source)) {
        for (int i = 0; i < rowCount(source); i++) {
            QModelIndex curNewParentIndexChild = source.child(i, 0);
            copyIndexToNewParent(curNewParentIndexChild, newParentIndex, pMode);
        }
    }

    return newParentIndex;
}

void WBDocumentTreeModel::moveIndexes(const QModelIndexList &source, const QModelIndex &destination)
{
    QModelIndex destinationParent = destination;
    while(!isCatalog(destinationParent)){
        destinationParent = destinationParent.parent();
    }

    WBDocumentTreeNode *newParentNode = nodeFromIndex(destinationParent);

    bool hasOneInsertion = false;

    for(int i = 0; i < source.size(); i++){
        WBDocumentTreeNode *sourceNode = nodeFromIndex(source.at(i));
        QModelIndex s = source.at(i);

        if(newParentNode == sourceNode->parentNode() || sourceNode->findNode(newParentNode))
            continue;

        if(s.internalId() != destinationParent.internalId()){
            int sourceIndex = source.at(i).row();
            int destIndex = positionForParent(sourceNode, newParentNode);

            beginMoveRows(s.parent(), sourceIndex, sourceIndex, destinationParent, destIndex);
            fixNodeName(s, destinationParent);
            sourceNode->parentNode()->moveChild(sourceNode, destIndex, newParentNode);
            updateIndexNameBindings(sourceNode);
            hasOneInsertion = true;
        }
    }

    if(hasOneInsertion)
        endMoveRows();
}

void WBDocumentTreeModel::moveIndex(const QModelIndex &what, const QModelIndex &destination)
{
    QModelIndexList list;
    list.push_back(what);
    moveIndexes(list, destination);
}

void WBDocumentTreeModel::setCurrentDocument(WBDocumentProxy *pDocument)
{
    WBDocumentTreeNode *testCurNode = findProxy(pDocument, mRootNode);

    if (testCurNode) {
        setCurrentNode(testCurNode);
    }
}

QModelIndex WBDocumentTreeModel::indexForProxy(WBDocumentProxy *pSearch) const
{
    WBDocumentTreeNode *proxy = findProxy(pSearch, mRootNode);
    if (!proxy) {
        return QModelIndex();
    }

    return indexForNode(proxy);
}

void WBDocumentTreeModel::setRootNode(WBDocumentTreeNode *pRoot)
{
    mRootNode = pRoot;
    //reset();
}

WBDocumentProxy *WBDocumentTreeModel::proxyForIndex(const QModelIndex &pIndex) const
{
    WBDocumentTreeNode *node = nodeFromIndex(pIndex);
    if (!node) {
        return 0;
    }

    return node->proxyData();
}

QString WBDocumentTreeModel::virtualDirForIndex(const QModelIndex &pIndex) const
{
    QString result;
    WBDocumentTreeNode *curNode = nodeFromIndex(pIndex);
    //protect the 2nd level items
    while (curNode->parentNode() && !curNode->isTopLevel()) {
        result.prepend(curNode->parentNode()->nodeName() + "/");
        curNode = curNode->parentNode();
    }

    if (result.endsWith("/")) {
        result.truncate(result.count() - 1);
    }

    return result;
}

QString WBDocumentTreeModel::virtualPathForIndex(const QModelIndex &pIndex) const
{
    WBDocumentTreeNode *curNode = nodeFromIndex(pIndex);
    Q_ASSERT(curNode);

    return virtualDirForIndex(pIndex) + "/" + curNode->nodeName();
}

QStringList WBDocumentTreeModel::nodeNameList(const QModelIndex &pIndex) const
{
    QStringList result;

    WBDocumentTreeNode *catalog = nodeFromIndex(pIndex);
    if (catalog->nodeType() != WBDocumentTreeNode::Catalog) {
        return QStringList();
    }

    foreach (WBDocumentTreeNode *curNode, catalog->children()) {
        result << curNode->nodeName();
    }

    return result;
}

bool WBDocumentTreeModel::newNodeAllowed(const QModelIndex &pSelectedIndex)  const
{
    if (!pSelectedIndex.isValid()) {
        return false;
    }

    if (inTrash(pSelectedIndex) || pSelectedIndex == trashIndex()) {
        return false;
    }

    return true;
}

QModelIndex WBDocumentTreeModel::goTo(const QString &dir)
{
    QStringList pathList = dir.split("/", QString::SkipEmptyParts);

    if (pathList.isEmpty()) {
        return untitledDocumentsIndex();
    }

    if (pathList.first() != WBPersistenceManager::myDocumentsName
            && pathList.first() != WBSettings::trashedDocumentGroupNamePrefix
            && pathList.first() != WBPersistenceManager::modelsName) {
        pathList.prepend(WBPersistenceManager::myDocumentsName);
    }

    QModelIndex parentIndex;

    bool searchingNode = true;
    while (!pathList.isEmpty())
    {
        QString curLevelName = pathList.takeFirst();
        if (searchingNode) {
            searchingNode = false;
            for (int i = 0; i < rowCount(parentIndex); ++i) {
                QModelIndex curChildIndex = index(i, 0, parentIndex);
                if (nodeFromIndex(curChildIndex)->nodeName() == curLevelName) {
                    searchingNode = true;
                    parentIndex = curChildIndex;
                    break;
                }
            }
        }

        if (!searchingNode) {
            WBDocumentTreeNode *newChild = new WBDocumentTreeNode(WBDocumentTreeNode::Catalog, curLevelName);
            parentIndex = addNode(newChild, parentIndex);
        }
    }

    return parentIndex;
}

bool WBDocumentTreeModel::inTrash(const QModelIndex &index) const
{
    return isDescendantOf(index, trashIndex());
}

bool WBDocumentTreeModel::inUntitledDocuments(const QModelIndex &index) const
{
    return isDescendantOf(index, untitledDocumentsIndex());
}

bool WBDocumentTreeModel::inMyDocuments(const QModelIndex &index) const
{
    return isDescendantOf(index, myDocumentsIndex());
}

void WBDocumentTreeModel::addDocument(WBDocumentProxy *pProxyData, const QModelIndex &pParent)
{
    if (!pProxyData) {
        return;
    }
    QString docName = pProxyData->metaData(WBSettings::documentName).toString();
    QString docGroupName = pProxyData->metaData(WBSettings::documentGroupName).toString();

    if (docName.isEmpty()) {
        return;
    }

    QModelIndex lParent = pParent;
    WBDocumentTreeNode *freeNode = new WBDocumentTreeNode(WBDocumentTreeNode::Document
                                                          , docName
                                                          , QString()
                                                          , pProxyData);
    if (!pParent.isValid()) {
        lParent = goTo(docGroupName);
    }

    addNode(freeNode, lParent);
}

void WBDocumentTreeModel::addNewDocument(WBDocumentProxy *pProxyData, const QModelIndex &pParent)
{
    addDocument(pProxyData, pParent);
    mNewDocuments << pProxyData;
}

QModelIndex WBDocumentTreeModel::addCatalog(const QString &pName, const QModelIndex &pParent)
{
    if (pName.isEmpty() || !pParent.isValid()) {
        return QModelIndex();
    }

    WBDocumentTreeNode *catalogNode = new WBDocumentTreeNode(WBDocumentTreeNode::Catalog, pName);
    return addNode(catalogNode, pParent);
}

void WBDocumentTreeModel::setNewName(const QModelIndex &index, const QString &newName)
{
    if (!index.isValid()) {
        return;
    }

    WBDocumentTreeNode *indexNode = nodeFromIndex(index);

    QString magicSeparator = "+!##s";
    if (isCatalog(index)) {
        QString fullNewName = newName;
        fullNewName.replace('/', '-');
        if (!newName.contains(magicSeparator)) {
            indexNode->setNodeName(fullNewName);
            QString virtualDir = virtualDirForIndex(index);
            fullNewName.prepend(virtualDir.isEmpty() ? "" : virtualDir + magicSeparator);
        }
        for (int i = 0; i < rowCount(index); i++) {
            QModelIndex subIndex = this->index(i, 0, index);
            setNewName(subIndex, fullNewName + magicSeparator + subIndex.data().toString());
        }

    } else if (isDocument(index)) {
        Q_ASSERT(indexNode->proxyData());

        int prefixIndex = newName.lastIndexOf(magicSeparator);
        if (prefixIndex != -1) {
            QString newDocumentGroupName = newName.left(prefixIndex).replace(magicSeparator, "/");
            indexNode->proxyData()->setMetaData(WBSettings::documentGroupName, newDocumentGroupName);
        } else {
            indexNode->setNodeName(newName);
            indexNode->proxyData()->setMetaData(WBSettings::documentName, newName);
        }

        WBPersistenceManager::persistenceManager()->persistDocumentMetadata(indexNode->proxyData());
    }
}

QString WBDocumentTreeModel::adjustNameForParentIndex(const QString &pName, const QModelIndex &pIndex)
{
    int i = 0;
    QString newName = pName;
    QStringList siblingNames = nodeNameList(pIndex);
    while (siblingNames.contains(newName)) {
        newName = pName + " " + QVariant(++i).toString();
    }

    return newName;
}

void WBDocumentTreeModel::fixNodeName(const QModelIndex &source, const QModelIndex &dest)
{
    // Determine whether to provide a name with postfix if the name in current level allready exists
    WBDocumentTreeNode *srcNode = nodeFromIndex(source);
    Q_ASSERT(srcNode);

    QString newName = srcNode->nodeName();
    if (source.parent() != dest
            && (dest != trashIndex()
            || !inTrash(dest))) {
        newName = adjustNameForParentIndex(newName, dest);
        srcNode->setNodeName(newName);
        nodeFromIndex(source)->setNodeName(newName);
    }
}

void WBDocumentTreeModel::updateIndexNameBindings(WBDocumentTreeNode *nd)
{
    Q_ASSERT(nd);

    if (nd->nodeType() == WBDocumentTreeNode::Catalog) {
        foreach (WBDocumentTreeNode *lnd, nd->children()) {
            updateIndexNameBindings(lnd);
        }
    } else if (nd->proxyData()) {
        nd->proxyData()->setMetaData(WBSettings::documentGroupName, virtualPathForIndex(indexForNode(nd->parentNode())));
        nd->proxyData()->setMetaData(WBSettings::documentName, nd->nodeName());
        nd->proxyData()->setMetaData(WBSettings::documentUpdatedAt, WBStringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
        WBPersistenceManager::persistenceManager()->persistDocumentMetadata(nd->proxyData());
    }
}

bool WBDocumentTreeModel::isDescendantOf(const QModelIndex &pPossibleDescendant, const QModelIndex &pPossibleAncestor) const
{
    if (!pPossibleDescendant.isValid()) {
        return false;
    }

    QModelIndex ancestor = pPossibleDescendant;
    while (ancestor.parent().isValid()) {
        ancestor = ancestor.parent();
        if (ancestor == pPossibleAncestor) {
            return true;
        }
    }

    return false;
}

QModelIndex WBDocumentTreeModel::addNode(WBDocumentTreeNode *pFreeNode, const QModelIndex &pParent, eAddItemMode pMode)
{
    WBDocumentTreeNode *tstParent = nodeFromIndex(pParent);

    if (!pParent.isValid() || tstParent->nodeType() != WBDocumentTreeNode::Catalog) {
        return QModelIndex();
    }
    int newIndex = pMode == aDetectPosition ? positionForParent(pFreeNode, tstParent): tstParent->children().size();
    beginInsertRows(pParent, newIndex, newIndex);
    tstParent->insertChild(newIndex, pFreeNode);
    endInsertRows();

    return createIndex(newIndex, 0, pFreeNode);
}

int WBDocumentTreeModel::positionForParent(WBDocumentTreeNode *pFreeNode, WBDocumentTreeNode *pParentNode)
{
    Q_ASSERT(pFreeNode);
    Q_ASSERT(pParentNode);
    Q_ASSERT(pParentNode->nodeType() == WBDocumentTreeNode::Catalog);

    int c = -1;
    int childCount = pParentNode->children().count();
    while (c <= childCount) {
        if (++c == childCount || lessThan(pFreeNode, pParentNode->children().at(c))) {
            break;
        }
    }
    return c == -1 ? childCount : c;
}

WBDocumentTreeNode *WBDocumentTreeModel::nodeFromIndex(const QModelIndex &pIndex) const
{
    if (pIndex.isValid()) {
        return static_cast<WBDocumentTreeNode*>(pIndex.internalPointer());
    } else {
        return mRootNode;
    }
}

bool WBDocumentTreeModel::nodeLessThan(const WBDocumentTreeNode *firstIndex, const WBDocumentTreeNode *secondIndex)
{
    return firstIndex->nodeName() < secondIndex->nodeName();
}

WBDocumentTreeModel::~WBDocumentTreeModel()
{
    delete mRootNode;
}

WBDocumentTreeView::WBDocumentTreeView(QWidget *parent) : QTreeView(parent)
{
    setObjectName("WBDocumentTreeView");
    setRootIsDecorated(true);
}

void WBDocumentTreeView::setSelectedAndExpanded(const QModelIndex &pIndex, bool pExpand, bool pEdit)
{
    if (!pIndex.isValid()) {
        return;
    }

    QModelIndex indexCurrentDoc = pIndex;
    clearSelection();

    WBSortFilterProxyModel *proxy = dynamic_cast<WBSortFilterProxyModel*>(model());

    QItemSelectionModel::SelectionFlags sel = pExpand
                                                ? QItemSelectionModel::Select
                                                : QItemSelectionModel::Deselect;

    setCurrentIndex(pExpand
                    ? indexCurrentDoc
                    : QModelIndex());

    selectionModel()->select(proxy->mapFromSource(indexCurrentDoc), QItemSelectionModel::Rows | sel);

    while (indexCurrentDoc.parent().isValid()) {
        setExpanded(indexCurrentDoc.parent(), pExpand);
        indexCurrentDoc = indexCurrentDoc.parent();
    }

    scrollTo(proxy->mapFromSource(pIndex), QAbstractItemView::PositionAtCenter);

    if (pEdit)
        edit(proxy->mapFromSource(pIndex));
}

void WBDocumentTreeView::onModelIndexChanged(const QModelIndex &pNewIndex, const QModelIndex &pOldIndex)
{
    Q_UNUSED(pOldIndex)

    QModelIndex indexSource = mapIndexToSource(pNewIndex);

    setSelectedAndExpanded(indexSource, true);
}

void WBDocumentTreeView::hSliderRangeChanged(int min, int max)
{
    Q_UNUSED(min);
    Q_UNUSED(max);

    QScrollBar *hScroller = horizontalScrollBar();
    if (hScroller)
    {
        hScroller->triggerAction(QAbstractSlider::SliderToMaximum);
    }
}

void WBDocumentTreeView::dragEnterEvent(QDragEnterEvent *event)
{
    QTreeView::dragEnterEvent(event);
    event->accept();
    event->acceptProposedAction();
}

void WBDocumentTreeView::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event);

    WBDocumentTreeModel *docModel = 0;

    WBSortFilterProxyModel *proxy = dynamic_cast<WBSortFilterProxyModel*>(model());

    if(proxy){
        docModel = dynamic_cast<WBDocumentTreeModel*>(proxy->sourceModel());
    }else{
        docModel =  dynamic_cast<WBDocumentTreeModel*>(model());
    }

    docModel->setHighLighted(QModelIndex());
    update();
}

void WBDocumentTreeView::dragMoveEvent(QDragMoveEvent *event)
{
    QModelIndex index;
    if (selectedIndexes().count() > 0)
    {
        index = selectedIndexes().first();
    }

    bool acceptIt = isAcceptable(index, indexAt(event->pos()));

    if (event->mimeData()->hasFormat(WBApplication::mimeTypeUniboardPage)) {
        WBSortFilterProxyModel *proxy = dynamic_cast<WBSortFilterProxyModel*>(model());

        WBDocumentTreeModel *docModel = 0;

        if(proxy){
            docModel = dynamic_cast<WBDocumentTreeModel*>(proxy->sourceModel());
        }else{
            docModel =  dynamic_cast<WBDocumentTreeModel*>(model());
        }

        QModelIndex targetIndex = mapIndexToSource(indexAt(event->pos()));

        if (!docModel || !docModel->isDocument(targetIndex) || docModel->inTrash(targetIndex)) {
            event->ignore();
            event->setDropAction(Qt::IgnoreAction);
            docModel->setHighLighted(QModelIndex());
            acceptIt = false;
        } else {
            docModel->setHighLighted(targetIndex);
            acceptIt = true;
        }
        updateIndexEnvirons(indexAt(event->pos()));
    }
    QTreeView::dragMoveEvent(event);

    event->setAccepted(acceptIt);
}

void WBDocumentTreeView::dropEvent(QDropEvent *event)
{
    event->ignore();
    event->setDropAction(Qt::IgnoreAction);
    WBDocumentTreeModel *docModel = 0;

    WBSortFilterProxyModel *proxy = dynamic_cast<WBSortFilterProxyModel*>(model());
    if(proxy){
        docModel = dynamic_cast<WBDocumentTreeModel*>(proxy->sourceModel());
    }

    QModelIndex targetIndex = mapIndexToSource(indexAt(event->pos()));
    QModelIndexList dropIndex = mapIndexesToSource(selectedIndexes());

    //clear the selection right after
    //selectionModel()->clearSelection();

    bool isUBPage = event->mimeData()->hasFormat(WBApplication::mimeTypeUniboardPage);

    bool targetIsInTrash = docModel->inTrash(targetIndex) || docModel->trashIndex() == targetIndex;
    bool targetIsInMyDocuments = docModel->inMyDocuments(targetIndex) || docModel->myDocumentsIndex() == targetIndex;

    if (!targetIsInMyDocuments && !targetIsInTrash)
        return;

    if (isUBPage)
    {
        WBDocumentProxy *targetDocProxy = docModel->proxyData(targetIndex);

        const WBMimeData *ubMime = qobject_cast <const WBMimeData*>(event->mimeData());
        if (!targetDocProxy || !ubMime || !ubMime->items().count()) {
            qDebug() << "an error ocured while parsing " << WBApplication::mimeTypeUniboardPage;
            QTreeView::dropEvent(event);
            return;
        }

        int count = 0;
        int total = ubMime->items().size();
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

        foreach (WBMimeDataItem sourceItem, ubMime->items())
        {
            WBDocumentProxy *fromProxy = sourceItem.documentProxy();
            int fromIndex = sourceItem.sceneIndex();
            int toIndex = targetDocProxy->pageCount();            

            count++;

            WBApplication::applicationController->showMessage(tr("Copying page %1/%2").arg(count).arg(total), true);

            WBGraphicsScene *scene = WBPersistenceManager::persistenceManager()->loadDocumentScene(sourceItem.documentProxy(), sourceItem.sceneIndex());
            if (scene)
            {
                WBGraphicsScene* sceneClone = scene->sceneDeepCopy();

                WBDocumentProxy *targetDocProxy = docModel->proxyForIndex(targetIndex);

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

                QString thumbTmp(fromProxy->persistencePath() + WBFileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", fromIndex));
                QString thumbTo(targetDocProxy->persistencePath() + WBFileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", toIndex));

                QFile::remove(thumbTo);
                QFile::copy(thumbTmp, thumbTo);

                Q_ASSERT(QFileInfo(thumbTmp).exists());
                Q_ASSERT(QFileInfo(thumbTo).exists());
                const QPixmap *pix = new QPixmap(thumbTmp);
                WBDocumentController *ctrl = WBApplication::documentController;
                ctrl->addPixmapAt(pix, toIndex);
                ctrl->TreeViewSelectionChanged(ctrl->firstSelectedTreeIndex(), QModelIndex());
            }

            QApplication::restoreOverrideCursor();
            WBApplication::applicationController->showMessage(tr("%1 pages copied", "", total).arg(total), false);

            docModel->setHighLighted(QModelIndex());
        }

    }
    else
    {
        if(targetIsInTrash)
        {
            if (!WBApplication::mainWindow->yesNoQuestion(tr("Remove Item"), tr("Are you sure you want to remove the selected item(s) ?")))
                return;

            WBApplication::documentController->moveIndexesToTrash(dropIndex, docModel);
        }else{
            docModel->moveIndexes(dropIndex, targetIndex);
        }
    }

    expand(proxy->mapFromSource(targetIndex));

    QTreeView::dropEvent(event);

    WBApplication::documentController->updateActions();
}

void WBDocumentTreeView::paintEvent(QPaintEvent *event)
{
    QTreeView::paintEvent(event);
}

void WBDocumentTreeView::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    QTreeView::rowsAboutToBeRemoved(parent, start, end);
}

bool WBDocumentTreeView::isAcceptable(const QModelIndex &dragIndex, const QModelIndex &atIndex)
{
    QModelIndex dragIndexSource = mapIndexToSource(dragIndex);
    QModelIndex atIndexSource = mapIndexToSource(atIndex);

    if (!dragIndexSource.isValid()) {
        return false;
    }

    return true;
}

Qt::DropAction WBDocumentTreeView::acceptableAction(const QModelIndex &dragIndex, const QModelIndex &atIndex)
{    
    return Qt::MoveAction;
}

void WBDocumentTreeView::updateIndexEnvirons(const QModelIndex &index)
{
    QRect updateRect = visualRect(index);
    const int multipler = 3;
    updateRect.adjust(0, -updateRect.height() * multipler, 0, updateRect.height() * multipler);
    update(updateRect);
}

QModelIndex WBDocumentTreeView::mapIndexToSource(const QModelIndex &index)
{
    WBSortFilterProxyModel *proxy = dynamic_cast<WBSortFilterProxyModel*>(model());

    if(proxy){
        return proxy->mapToSource(index);
    }

    return index;
}

QModelIndexList WBDocumentTreeView::mapIndexesToSource(const QModelIndexList &indexes)
{
    WBSortFilterProxyModel *proxy = dynamic_cast<WBSortFilterProxyModel*>(model());

    if(proxy){
        QModelIndexList list;

        for(int i = 0; i < indexes.size(); i++){
            list.push_back(proxy->mapToSource(indexes.at(i)));
        }

        return list;
    }

    return indexes;
}

WBDocumentTreeItemDelegate::WBDocumentTreeItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{

}

void WBDocumentTreeItemDelegate::commitAndCloseEditor()
{
    QLineEdit *lineEditor = qobject_cast<QLineEdit*>(sender());
    if (lineEditor) {
        emit commitData(lineEditor);
        //emit closeEditor(lineEditor);
        emit WBApplication::documentController->reorderDocumentsRequested();
    }
}

void WBDocumentTreeItemDelegate::processChangedText(const QString &str) const
{
    QLineEdit *editor = qobject_cast<QLineEdit*>(sender());
    if (!editor) {
        return;
    }

    if (!validateString(str)) {
        editor->setStyleSheet("background-color: #FFB3C8;");
    } else {
        editor->setStyleSheet("background-color: #FFFFFF;");
    }
}

bool WBDocumentTreeItemDelegate::validateString(const QString &str) const
{
    return !mExistingFileNames.contains(str);
}

QWidget *WBDocumentTreeItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);

    if(index.column() == 0){
        mExistingFileNames.clear();
        const WBDocumentTreeModel *indexModel = qobject_cast<const WBDocumentTreeModel*>(index.model());
        if (indexModel) {
            mExistingFileNames = indexModel->nodeNameList(index.parent());
            mExistingFileNames.removeOne(index.data().toString());
        }

        QLineEdit *nameEditor = new QLineEdit(parent);
        connect(nameEditor, SIGNAL(editingFinished()), this, SLOT(commitAndCloseEditor()));
        connect(nameEditor, SIGNAL(textChanged(QString)), this, SLOT(processChangedText(QString)));
        return nameEditor;
    }

    //N/C - NNe - 20140407 : the other column are not editable.
    return 0;
}

void WBDocumentTreeItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    if (index.column() == 0) {
        QLineEdit *lineEditor = qobject_cast<QLineEdit*>(editor);
        lineEditor->setText(index.data().toString());
        lineEditor->selectAll();
    }
}

void WBDocumentTreeItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QLineEdit *lineEditor = qobject_cast<QLineEdit*>(editor);
    if (validateString(lineEditor->text())) {
        model->setData(index, lineEditor->text());
    }
}

void WBDocumentTreeItemDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter, option, index);
}

WBDocumentController::WBDocumentController(WBMainWindow* mainWindow)
   : WBDocumentContainer(mainWindow->centralWidget())
   , mSelectionType(None)
   , mParentWidget(mainWindow->centralWidget())
   , mBoardController(WBApplication::boardController)
   , mDocumentUI(0)
   , mMainWindow(mainWindow)
   , mDocumentWidget(0)
   , mIsClosing(false)
   , mToolsPalette(0)
   , mToolsPalettePositionned(false)
   , mDocumentTrashGroupName(tr("Trash"))
   , mDefaultDocumentGroupName(tr("Untitled Documents"))
   , mCurrentIndexMoved(false)
{

    setupViews();
    setupToolbar();
    connect(this, SIGNAL(exportDone()), mMainWindow, SLOT(onExportDone()));
    connect(this, SIGNAL(documentThumbnailsUpdated(WBDocumentContainer*)), this, SLOT(refreshDocumentThumbnailsView(WBDocumentContainer*)));
    connect(this, SIGNAL(reorderDocumentsRequested()), this, SLOT(reorderDocuments()));
}

WBDocumentController::~WBDocumentController()
{
   if (mDocumentUI)
       delete mDocumentUI;
}

void WBDocumentController::createNewDocument()
{
    WBPersistenceManager *pManager = WBPersistenceManager::persistenceManager();
    WBDocumentTreeModel *docModel = pManager->mDocumentTreeStructureModel;
    QModelIndex selectedIndex = firstSelectedTreeIndex();

    if (!selectedIndex.isValid())
        selectedIndex = docModel->myDocumentsIndex();

    QString groupName = docModel->isCatalog(selectedIndex)
                ? docModel->virtualPathForIndex(selectedIndex)
                : docModel->virtualDirForIndex(selectedIndex);

    WBDocumentProxy *document = pManager->createDocument(groupName);
    selectDocument(document, true, false, true);

    if (document)
        pManager->mDocumentTreeStructureModel->markDocumentAsNew(document);
}

void WBDocumentController::selectDocument(WBDocumentProxy* proxy, bool setAsCurrentDocument, const bool onImport, const bool editMode)
{
    if (proxy==NULL)
    {
        setDocument(NULL);
        return;
    }

    if (setAsCurrentDocument) {
        WBPersistenceManager::persistenceManager()->mDocumentTreeStructureModel->setCurrentDocument(proxy);
        QModelIndex indexCurrentDoc = WBPersistenceManager::persistenceManager()->mDocumentTreeStructureModel->indexForProxy(proxy);
        mDocumentUI->documentTreeView->setSelectedAndExpanded(indexCurrentDoc, true, editMode);

        if (proxy != mBoardController->selectedDocument()) // only if wanted Document is different from document actually on Board,
        {
            mBoardController->setActiveDocumentScene(proxy, 0, true, onImport);
        }
    }

    mSelectionType = Document;
    setDocument(proxy);
}

void WBDocumentController::createNewDocumentGroup()
{
    WBPersistenceManager *pManager = WBPersistenceManager::persistenceManager();
    WBDocumentTreeModel *docModel = pManager->mDocumentTreeStructureModel;
    QModelIndex selectedIndex = firstSelectedTreeIndex();
    if (!selectedIndex.isValid()) {
        selectedIndex = docModel->myDocumentsIndex();
    }
    QModelIndex parentIndex = docModel->isCatalog(selectedIndex)
            ? selectedIndex
            : selectedIndex.parent();

    QString newFolderName = docModel->adjustNameForParentIndex(tr("New Folder"), parentIndex);

    QModelIndex newIndex = docModel->addCatalog(newFolderName, parentIndex);
    mDocumentUI->documentTreeView->setSelectedAndExpanded(newIndex, true, true);
}


WBDocumentProxy* WBDocumentController::selectedDocumentProxy()
{
    return WBPersistenceManager::persistenceManager()->mDocumentTreeStructureModel->proxyForIndex(firstSelectedTreeIndex());
}

QList<WBDocumentProxy*> WBDocumentController::selectedProxies()
{
    QList<WBDocumentProxy*> result;

    foreach (QModelIndex curIndex, mapIndexesToSource(mDocumentUI->documentTreeView->selectionModel()->selectedIndexes())) {
        result << WBPersistenceManager::persistenceManager()->mDocumentTreeStructureModel->proxyForIndex(curIndex);
    }

    return result;
}

QModelIndexList WBDocumentController::selectedTreeIndexes()
{
    return mapIndexesToSource(mDocumentUI->documentTreeView->selectionModel()->selectedRows(0));
}

WBDocumentProxy* WBDocumentController::firstSelectedTreeProxy()
{
    return selectedProxies().count() ? selectedProxies().first() : 0;
}

void WBDocumentController::TreeViewSelectionChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous)

    WBDocumentTreeModel *docModel = WBPersistenceManager::persistenceManager()->mDocumentTreeStructureModel;

    QModelIndex current_index = mapIndexToSource(current);

    //if the selection contains more than one object, don't show the thumbnail.
    //We have just to pass a null proxy to disable the display of thumbnail
    WBDocumentProxy *currentDocumentProxy = 0;

    if(current_index.isValid() && mDocumentUI->documentTreeView->selectionModel()->selectedRows(0).size() == 1){
        currentDocumentProxy = docModel->proxyData(current_index);
        setDocument(currentDocumentProxy, false);
    }

    if (mCurrentIndexMoved) {
        if (docModel->isDocument(current_index)) {
            docModel->setCurrentDocument(currentDocumentProxy);
        } else if (docModel->isCatalog(current_index)) {
            docModel->setCurrentDocument(0);
        }
        mCurrentIndexMoved = false;
    }

    itemSelectionChanged(docModel->isCatalog(current_index) ? Folder : Document);
}

QModelIndex WBDocumentController::mapIndexToSource(const QModelIndex &index)
{
    WBSortFilterProxyModel *proxy = dynamic_cast<WBSortFilterProxyModel*>(mDocumentUI->documentTreeView->model());

    if(proxy){
        return proxy->mapToSource(index);
    }

    return index;
}

QModelIndexList WBDocumentController::mapIndexesToSource(const QModelIndexList &indexes)
{
    WBSortFilterProxyModel *proxy = dynamic_cast<WBSortFilterProxyModel*>(mDocumentUI->documentTreeView->model());

    if(proxy){
        QModelIndexList list;

        for(int i = 0; i < indexes.size(); i++){
            list.push_back(proxy->mapToSource(indexes.at(i)));
        }

        return list;
    }

    return indexes;
}

void WBDocumentController::TreeViewSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected)

    int nbIndexes = selected.indexes().count();

    if (nbIndexes) {
        QModelIndexList list = mDocumentUI->documentTreeView->selectionModel()->selectedRows();

        //if multi-selection
        if(list.count() > 1)
        {
            for (int i=0; i < list.count() ; i++)
            {
                QModelIndex newSelectedRow = list.at(i);
                QModelIndex parent = newSelectedRow.parent();
                bool isParentIsSelected = false;
                while(parent.isValid())
                {
                    isParentIsSelected |= (list.indexOf(parent) != -1);

                    if(isParentIsSelected)
                        break;

                    parent = parent.parent();
                }

                if(!isParentIsSelected)
                    TreeViewSelectionChanged(newSelectedRow, QModelIndex());
                else
                    mDocumentUI->documentTreeView->selectionModel()->select(newSelectedRow, QItemSelectionModel::Deselect | QItemSelectionModel::Rows);

            }
        }
        else
        {
            TreeViewSelectionChanged(selected.indexes().at(0), QModelIndex());
        }
    }
}

void WBDocumentController::itemSelectionChanged(LastSelectedElementType newSelection)
{
    mSelectionType = newSelection;
    updateActions();
}


void WBDocumentController::setupViews()
{
    if (!mDocumentWidget)
    {
        mDocumentWidget = new QWidget(mMainWindow->centralWidget());
        mMainWindow->addDocumentsWidget(mDocumentWidget);

        mDocumentUI = new Ui::documents();

        mDocumentUI->setupUi(mDocumentWidget);

		int thumbWidth = WBSettings::settings()->documentThumbnailWidth->get().toInt();

        mDocumentUI->documentZoomSlider->setValue(thumbWidth);
        mDocumentUI->thumbnailWidget->setThumbnailWidth(thumbWidth);

        connect(mDocumentUI->documentZoomSlider, SIGNAL(valueChanged(int)), this,
                SLOT(documentZoomSliderValueChanged(int)));

        connect(mMainWindow->actionOpen, SIGNAL(triggered()), this, SLOT(openSelectedItem()));
        connect(mMainWindow->actionNewFolder, SIGNAL(triggered()), this, SLOT(createNewDocumentGroup()));
        connect(mMainWindow->actionNewDocument, SIGNAL(triggered()), this, SLOT(createNewDocument()));

        connect(mMainWindow->actionImport, SIGNAL(triggered(bool)), this, SLOT(importFile()));

        QMenu* addMenu = new QMenu(mDocumentWidget);
        mAddFolderOfImagesAction = addMenu->addAction(tr("Add Folder of Images"));
        mAddImagesAction = addMenu->addAction(tr("Add Images"));
        mAddFileToDocumentAction = addMenu->addAction(tr("Add Pages from File"));

        connect(mAddFolderOfImagesAction, SIGNAL(triggered(bool)), this, SLOT(addFolderOfImages()));
        connect(mAddFileToDocumentAction, SIGNAL(triggered(bool)), this, SLOT(addFileToDocument()));
        connect(mAddImagesAction, SIGNAL(triggered(bool)), this, SLOT(addImages()));

        foreach (QWidget* menuWidget,  mMainWindow->actionDocumentAdd->associatedWidgets())
        {
            QToolButton *tb = qobject_cast<QToolButton*>(menuWidget);

            if (tb && !tb->menu())
            {
                tb->setObjectName("ubButtonMenu");
                tb->setPopupMode(QToolButton::InstantPopup);

                QMenu* menu = new QMenu(mDocumentWidget);

                menu->addAction(mAddFolderOfImagesAction);
                menu->addAction(mAddImagesAction);
                menu->addAction(mAddFileToDocumentAction);

                tb->setMenu(menu);
            }
        }

        QMenu* exportMenu = new QMenu(mDocumentWidget);

        WBDocumentManager *documentManager = WBDocumentManager::documentManager();
        for (int i = 0; i < documentManager->supportedExportAdaptors().length(); i++)
        {
            WBExportAdaptor* adaptor = documentManager->supportedExportAdaptors()[i];
            QAction *currentExportAction = exportMenu->addAction(adaptor->exportName());
            currentExportAction->setData(i);
            connect(currentExportAction, SIGNAL(triggered (bool)), this, SLOT(exportDocument()));
            exportMenu->addAction(currentExportAction);
            adaptor->setAssociatedAction(currentExportAction);
        }

        foreach (QWidget* menuWidget,  mMainWindow->actionExport->associatedWidgets())
        {
            QToolButton *tb = qobject_cast<QToolButton*>(menuWidget);

            if (tb && !tb->menu())
            {
                tb->setObjectName("ubButtonMenu");
                tb->setPopupMode(QToolButton::InstantPopup);

                tb->setMenu(exportMenu);
            }
        }

#ifdef Q_OS_OSX
        mMainWindow->actionDelete->setShortcut(QKeySequence(Qt::Key_Backspace));
#else
        mMainWindow->actionDelete->setShortcut(QKeySequence(Qt::Key_Delete));
#endif

        connect(mMainWindow->actionDelete, SIGNAL(triggered()), this, SLOT(deleteSelectedItem()));
        connect(mMainWindow->actionDuplicate, SIGNAL(triggered()), this, SLOT(duplicateSelectedItem()));
        connect(mMainWindow->actionRename, SIGNAL(triggered()), this, SLOT(renameSelectedItem()));
        connect(mMainWindow->actionAddToWorkingDocument, SIGNAL(triggered()), this, SLOT(addToDocument()));

        WBDocumentTreeModel *model = WBPersistenceManager::persistenceManager()->mDocumentTreeStructureModel;

        mSortFilterProxyModel = new WBSortFilterProxyModel();

        mSortFilterProxyModel->setSourceModel(model);

        // sort documents according to preferences
        int sortKind  = WBSettings::settings()->documentSortKind->get().toInt();
        int sortOrder = WBSettings::settings()->documentSortOrder->get().toInt();

        // update icon and button
        mDocumentUI->sortKind->setCurrentIndex(sortKind);
        if (sortOrder == WBDocumentController::DESC)
            mDocumentUI->sortOrder->setChecked(true);

        mDocumentUI->documentTreeView->setModel(mSortFilterProxyModel);

        mDocumentUI->documentTreeView->setItemDelegate(new WBDocumentTreeItemDelegate(this));
        mDocumentUI->documentTreeView->setDragEnabled(true);
        mDocumentUI->documentTreeView->setAcceptDrops(true);
        mDocumentUI->documentTreeView->viewport()->setAcceptDrops(true);
        mDocumentUI->documentTreeView->setDropIndicatorShown(true);
        mDocumentUI->documentTreeView->header()->setStretchLastSection(false);
        mDocumentUI->documentTreeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
        mDocumentUI->documentTreeView->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        mDocumentUI->documentTreeView->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

        const int splitterLeftSize = WBSettings::settings()->documentSplitterLeftSize->get().toInt();
        const int splitterRightSize = WBSettings::settings()->documentSplitterRightSize->get().toInt();
        QList<int> splitterSizes;
        splitterSizes.append(splitterLeftSize);
        splitterSizes.append(splitterRightSize);
        mDocumentUI->splitter->setSizes(splitterSizes);

        //mDocumentUI->documentTreeView->hideColumn(1);
        mDocumentUI->documentTreeView->hideColumn(2);

        sortDocuments(sortKind, sortOrder);

        connect(mDocumentUI->sortKind, SIGNAL(activated(int)), this, SLOT(onSortKindChanged(int)));
        connect(mDocumentUI->sortOrder, SIGNAL(toggled(bool)), this, SLOT(onSortOrderChanged(bool)));

        connect(mDocumentUI->splitter, SIGNAL(splitterMoved(int,int)), this, SLOT(onSplitterMoved(int, int)));

        connect(mDocumentUI->documentTreeView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(TreeViewSelectionChanged(QItemSelection,QItemSelection)));
        connect(WBPersistenceManager::persistenceManager()->mDocumentTreeStructureModel, SIGNAL(indexChanged(QModelIndex,QModelIndex))
                ,mDocumentUI->documentTreeView, SLOT(onModelIndexChanged(QModelIndex,QModelIndex)));
        connect(WBPersistenceManager::persistenceManager()->mDocumentTreeStructureModel, SIGNAL(currentIndexMoved(QModelIndex,QModelIndex))
                ,this, SLOT(currentIndexMoved(QModelIndex,QModelIndex)));

        connect(mDocumentUI->thumbnailWidget, SIGNAL(sceneDropped(WBDocumentProxy*, int, int)), this, SLOT(moveSceneToIndex ( WBDocumentProxy*, int, int)));
        connect(mDocumentUI->thumbnailWidget, SIGNAL(resized()), this, SLOT(thumbnailViewResized()));
        connect(mDocumentUI->thumbnailWidget, SIGNAL(mouseDoubleClick(QGraphicsItem*,int)), this, SLOT(thumbnailPageDoubleClicked(QGraphicsItem*,int)));
        connect(mDocumentUI->thumbnailWidget, SIGNAL(mouseClick(QGraphicsItem*, int)), this, SLOT(pageClicked(QGraphicsItem*, int)));

        connect(mDocumentUI->thumbnailWidget->scene(), SIGNAL(selectionChanged()), this, SLOT(pageSelectionChanged()));

        connect(WBPersistenceManager::persistenceManager(), SIGNAL(documentSceneCreated(WBDocumentProxy*, int)), this, SLOT(documentSceneChanged(WBDocumentProxy*, int)));
        connect(WBPersistenceManager::persistenceManager(), SIGNAL(documentSceneWillBeDeleted(WBDocumentProxy*, int)), this, SLOT(documentSceneChanged(WBDocumentProxy*, int)));

        mDocumentUI->thumbnailWidget->setBackgroundBrush(WBSettings::documentViewLightColor);

        #ifdef Q_WS_MACX
            mMessageWindow = new WBMessageWindow(NULL);
        #else
            mMessageWindow = new WBMessageWindow(mDocumentUI->thumbnailWidget);
        #endif

        mMessageWindow->setCustomPosition(true);
        mMessageWindow->hide();
    }
}

void WBDocumentController::refreshDateColumns()
{
    if (WBSettings::settings()->documentSortKind->get().toInt() == WBDocumentController::Alphabetical)
    {
        if (!WBSettings::settings()->showDateColumnOnAlphabeticalSort->get().toBool())
        {
            mDocumentUI->documentTreeView->hideColumn(1);
            mDocumentUI->documentTreeView->hideColumn(2);
        }
        else
        {
            mDocumentUI->documentTreeView->showColumn(1);
            mDocumentUI->documentTreeView->hideColumn(2);
        }
    }
}

void WBDocumentController::reorderDocuments()
{
   int kindIndex = mDocumentUI->sortKind->currentIndex();
   int orderIndex = mDocumentUI->sortOrder->isChecked() ? WBDocumentController::DESC : WBDocumentController::ASC;

   sortDocuments(kindIndex, orderIndex);
}

void WBDocumentController::sortDocuments(int kind, int order)
{
    Qt::SortOrder sortOrder = Qt::AscendingOrder;
    if(order == WBDocumentController::DESC)
        sortOrder = Qt::DescendingOrder;

    if(kind == WBDocumentController::CreationDate){
        mSortFilterProxyModel->setSortRole(WBDocumentTreeModel::CreationDate);
        mSortFilterProxyModel->sort(1, sortOrder);
        mDocumentUI->documentTreeView->showColumn(1);
        mDocumentUI->documentTreeView->hideColumn(2);
    }else if(kind == WBDocumentController::UpdateDate){
        mSortFilterProxyModel->setSortRole(WBDocumentTreeModel::UpdateDate);
        mSortFilterProxyModel->sort(2, sortOrder);
        mDocumentUI->documentTreeView->hideColumn(1);
        mDocumentUI->documentTreeView->showColumn(2);
    }else{
        mSortFilterProxyModel->setSortRole(Qt::DisplayRole);
        mSortFilterProxyModel->sort(0, sortOrder);
        if (!WBSettings::settings()->showDateColumnOnAlphabeticalSort->get().toBool())
        {
            mDocumentUI->documentTreeView->hideColumn(1);
            mDocumentUI->documentTreeView->hideColumn(2);
        }
    }
}


void WBDocumentController::onSortOrderChanged(bool order)
{
    int kindIndex = mDocumentUI->sortKind->currentIndex();
    int orderIndex = order ? WBDocumentController::DESC : WBDocumentController::ASC;

    sortDocuments(kindIndex, orderIndex);

    WBSettings::settings()->documentSortOrder->setInt(orderIndex);
}

void WBDocumentController::onSortKindChanged(int index)
{
    int orderIndex = mDocumentUI->sortOrder->isChecked() ? WBDocumentController::DESC : WBDocumentController::ASC;

    sortDocuments(index, orderIndex);

    WBSettings::settings()->documentSortKind->setInt(index);
}

void WBDocumentController::onSplitterMoved(int size, int index)
{
    Q_UNUSED(index);
    WBSettings::settings()->documentSplitterLeftSize->setInt(size);
    WBSettings::settings()->documentSplitterRightSize->setInt(controlView()->size().width()-size);
}

QWidget* WBDocumentController::controlView()
{
    return mDocumentWidget;
}


void WBDocumentController::setupToolbar()
{
    WBApplication::app()->insertSpaceToToolbarBeforeAction(mMainWindow->documentToolBar, mMainWindow->actionBoard);
    connect(mMainWindow->actionDocumentTools, SIGNAL(triggered()), this, SLOT(toggleDocumentToolsPalette()));
}

void WBDocumentController::setupPalettes()
{

    mToolsPalette = new WBDocumentToolsPalette(controlView());

    mToolsPalette->hide();

    bool showToolsPalette = !mToolsPalette->isEmpty();
    mMainWindow->actionDocumentTools->setVisible(showToolsPalette);

    if (showToolsPalette)
    {
        mMainWindow->actionDocumentTools->trigger();
    }
}

void WBDocumentController::show()
{
    selectDocument(mBoardController->selectedDocument());

    //to be sure thumbnails will be up-to-date
    reloadThumbnails();

    updateActions();

    if(!mToolsPalette)
        setupPalettes();
}


void WBDocumentController::hide()
{
    // NOOP
}


void WBDocumentController::openSelectedItem()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    QList<QGraphicsItem*> selectedItems = mDocumentUI->thumbnailWidget->selectedItems();

    if (selectedItems.count() > 0)
    {
        WBSceneThumbnailPixmap* thumb = dynamic_cast<WBSceneThumbnailPixmap*> (selectedItems.last());

        if (thumb)
        {
            WBDocumentProxy* proxy = thumb->proxy();

            if (proxy && isOKToOpenDocument(proxy))
            {
                mBoardController->setActiveDocumentScene(proxy, thumb->sceneIndex());
                WBApplication::applicationController->showBoard();
            }
        }
    }
    else
    {
        WBDocumentProxy* proxy = selectedDocumentProxy();

        if (proxy && isOKToOpenDocument(proxy))
        {
            mBoardController->setActiveDocumentScene(proxy);
            WBApplication::applicationController->showBoard();
        }
    }

    QApplication::restoreOverrideCursor();
}

void WBDocumentController::duplicateSelectedItem()
{
    if (WBApplication::applicationController->displayMode() != WBApplicationController::Document)
        return;

    if (mSelectionType == Page)
    {
        QList<QGraphicsItem*> selectedItems = mDocumentUI->thumbnailWidget->selectedItems();
        QList<int> selectedSceneIndexes;
        foreach (QGraphicsItem *item, selectedItems)
        {
            WBSceneThumbnailPixmap *thumb = dynamic_cast<WBSceneThumbnailPixmap*>(item);
            if (thumb)
            {
                WBDocumentProxy *proxy = thumb->proxy();

                if (proxy)
                {
                    int sceneIndex = thumb->sceneIndex();
                    selectedSceneIndexes << sceneIndex;
                }
            }
        }
        if (selectedSceneIndexes.count() > 0)
        {
            duplicatePages(selectedSceneIndexes);
            emit documentThumbnailsUpdated(this);
            selectedDocument()->setMetaData(WBSettings::documentUpdatedAt, WBStringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
            WBMetadataDcSubsetAdaptor::persist(selectedDocument());
            int selectedThumbnail = selectedSceneIndexes.last() + selectedSceneIndexes.size();
            mDocumentUI->thumbnailWidget->selectItemAt(selectedThumbnail);
            int sceneCount = selectedSceneIndexes.count();
            showMessage(tr("duplicated %1 page","duplicated %1 pages",sceneCount).arg(sceneCount), false);

            mBoardController->setActiveDocumentScene(selectedThumbnail);
            mBoardController->reloadThumbnails();
        }
    }
    else
    {
        WBDocumentTreeModel *docModel = WBPersistenceManager::persistenceManager()->mDocumentTreeStructureModel;
        QModelIndex selectedIndex = firstSelectedTreeIndex();

        Q_ASSERT(!docModel->isConstant(selectedIndex) && !docModel->inTrash(selectedIndex));

        showMessage(tr("Duplicating Document %1").arg(""), true);

        docModel->copyIndexToNewParent(selectedIndex, selectedIndex.parent(), WBDocumentTreeModel::aContentCopy);

        showMessage(tr("Document %1 copied").arg(""), false);
    }

    emit reorderDocumentsRequested();
}

void WBDocumentController::deleteSelectedItem()
{
    WBDocumentTreeModel *docModel = WBPersistenceManager::persistenceManager()->mDocumentTreeStructureModel;

    if (!WBApplication::mainWindow->yesNoQuestion(tr("Remove Item"), tr("Are you sure you want to remove the selected item(s) ?")))
        return;

    QModelIndexList indexes = selectedTreeIndexes();

    if (indexes.size() > 1)
    {
        deleteMultipleItems(indexes, docModel);
    }
    else if (indexes.size() == 1)
    {
        deleteSingleItem(indexes.at(0), docModel);
    }

    updateActions();
}

void WBDocumentController::deleteMultipleItems(QModelIndexList indexes, WBDocumentTreeModel* docModel)
{
    DeletionType deletionForSelection = deletionTypeForSelection(mSelectionType, indexes.at(0), docModel);

    switch (deletionForSelection)
    {
        case DeletePage:
        {
            deletePages(mDocumentUI->thumbnailWidget->selectedItems());
            break;
        }
        case MoveToTrash:
        {
            moveIndexesToTrash(indexes, docModel);
            break;
        }
        case CompleteDelete:
        {
            for (int i =0; i < indexes.size(); i++)
            {
				WBDocumentTreeModel *docModel = WBPersistenceManager::persistenceManager()->mDocumentTreeStructureModel;
				WBDocumentProxy *newProxy = docModel->proxyData(indexes.at(i));
				if (newProxy) {
					pureSetDocument(newProxy);
				}
				emit documentThumbnailsUpdated(this);
				Sleep(100);//zhusizhi

                deleteIndexAndAssociatedData(indexes.at(i));
                
            }
            break;
        }
        case EmptyFolder:
        {
            for (int i =0; i < indexes.size(); i++)
            {
                QModelIndex currentIndex = indexes.at(i);
                if (currentIndex == docModel->myDocumentsIndex()) { //Emptying "My documents". Keeping Untitled Documents
                    int startInd = 0;
                    while (docModel->rowCount(currentIndex)) {
                        QModelIndex testSubINdecurrentIndex = docModel->index(startInd, 0, currentIndex);
                        if (testSubINdecurrentIndex == docModel->untitledDocumentsIndex()) {
                            emptyFolder(testSubINdecurrentIndex, MoveToTrash);
                            startInd++;
                            continue;
                        }
                        if (!testSubINdecurrentIndex.isValid()) {
                            break;
                        }
                        docModel->moveIndex(testSubINdecurrentIndex, docModel->trashIndex());
                    }
                    //Here, we are sure that the current scene has been deleted
                    createNewDocumentInUntitledFolder();
                } 
				else {
                    //Check if we will delete the current scene
                    WBDocumentTreeNode *currentNode = docModel->nodeFromIndex(currentIndex);
                    bool deleteCurrentScene = currentNode->findNode(docModel->nodeFromIndex(docModel->currentIndex()));

                    emptyFolder(currentIndex, MoveToTrash); //Empty constant folder

                    if(deleteCurrentScene) createNewDocumentInUntitledFolder();
                }
            }

            break;
        }
        case EmptyTrash:
        {
            for (int i=0; i < indexes.size(); i++)
            {
                emptyFolder(indexes.at(i), CompleteDelete); // Empty trash folder
            }

            break;
        }
    }
}

void WBDocumentController::deleteSingleItem(QModelIndex currentIndex, WBDocumentTreeModel* docModel)
{
    DeletionType deletionForSelection = deletionTypeForSelection(mSelectionType, currentIndex, docModel);

    switch (deletionForSelection)
    {
        case DeletePage:
        {
            deletePages(mDocumentUI->thumbnailWidget->selectedItems());
            break;
        }
        case MoveToTrash:
        {
            moveToTrash(currentIndex, docModel);
            break;
        }
        case CompleteDelete:
        {
			WBDocumentTreeModel *docModel = WBPersistenceManager::persistenceManager()->mDocumentTreeStructureModel;
			WBDocumentProxy *newProxy = docModel->proxyData(currentIndex);
			if (newProxy) {
				pureSetDocument(newProxy);
			}
			emit documentThumbnailsUpdated(this);
			Sleep(100);//zhusizhi
            deleteIndexAndAssociatedData(currentIndex);
            
            break;
        }
        case EmptyFolder:
        {
            if (currentIndex == docModel->myDocumentsIndex()) { //Emptying "My documents". Keeping Untitled Documents
                int startInd = 0;
                while (docModel->rowCount(currentIndex)) {
                    QModelIndex testSubINdecurrentIndex = docModel->index(startInd, 0, currentIndex);
                    if (testSubINdecurrentIndex == docModel->untitledDocumentsIndex()) {
                        emptyFolder(testSubINdecurrentIndex, MoveToTrash);
                        startInd++;
                        continue;
                    }
                    if (!testSubINdecurrentIndex.isValid()) {
                        break;
                    }
                    docModel->moveIndex(testSubINdecurrentIndex, docModel->trashIndex());
                }
                //Here, we are sure that the current scene has been deleted
                createNewDocumentInUntitledFolder();
            } 
			else {
                //Check if we will delete the current scene
                WBDocumentTreeNode *currentNode = docModel->nodeFromIndex(currentIndex);
                bool deleteCurrentScene = currentNode->findNode(docModel->nodeFromIndex(docModel->currentIndex()));

                emptyFolder(currentIndex, MoveToTrash); //Empty constant folder

                if(deleteCurrentScene) createNewDocumentInUntitledFolder();
            }

            break;
        }
        case EmptyTrash:
        {
            emptyFolder(currentIndex, CompleteDelete); // Empty trash folder
            break;
        }
    }
}

void WBDocumentController::moveIndexesToTrash(const QModelIndexList &list, WBDocumentTreeModel *docModel)
{
    QModelIndex currentScene = docModel->indexForNode(docModel->currentNode());

    //check if the current scene is selected
    QItemSelectionModel *selectionModel = mDocumentUI->documentTreeView->selectionModel();
    bool deleteCurrentScene = selectionModel->isSelected(mSortFilterProxyModel->mapFromSource(currentScene));

    //check if the current scene is in the hierarchy
    if(!deleteCurrentScene){
        for(int i = 0; i < list.size(); i++){
            deleteCurrentScene = docModel->isDescendantOf(currentScene, list.at(i));

            if(deleteCurrentScene){
                break;
            }
        }

    }

    QModelIndex proxyMapCurentScene = mSortFilterProxyModel->mapFromSource(currentScene);

    if(deleteCurrentScene){
        QModelIndex sibling = findPreviousSiblingNotSelected(proxyMapCurentScene, selectionModel);

        if(sibling.isValid()){
            QModelIndex sourceSibling = mSortFilterProxyModel->mapToSource(sibling);

            WBDocumentProxy *proxy = docModel->proxyForIndex(sourceSibling);

            if (proxy)
            {
                selectDocument(proxy,true);

                deleteCurrentScene = false;
            }
        }else{
            sibling = findNextSiblingNotSelected(proxyMapCurentScene, selectionModel);

            if(sibling.isValid()){
                QModelIndex sourceSibling = mSortFilterProxyModel->mapToSource(sibling);

                WBDocumentProxy *proxy = docModel->proxyForIndex(sourceSibling);

                if (proxy)
                {
                    selectDocument(proxy,true);

                    deleteCurrentScene = false;
                }
            }
        }
    }
    else
    {
        WBDocumentProxy* proxy = docModel->proxyForIndex(currentScene);
        selectDocument(proxy, true);
    }

    docModel->moveIndexes(list, docModel->trashIndex());

    if(deleteCurrentScene){
        createNewDocumentInUntitledFolder();
    }

    //selectionModel->clearSelection();
}

QModelIndex WBDocumentController::findPreviousSiblingNotSelected(const QModelIndex &index, QItemSelectionModel *selectionModel)
{
    QModelIndex sibling = index.sibling(index.row() - 1, 0);

    if(sibling.isValid())
    {
        if(!parentIsSelected(sibling, selectionModel)
                && !selectionModel->isSelected(sibling))
        {
            QModelIndex model = mSortFilterProxyModel->mapToSource(sibling);

            if(WBPersistenceManager::persistenceManager()->mDocumentTreeStructureModel->isCatalog(model))
            {
                return findPreviousSiblingNotSelected(sibling, selectionModel);
            }
            else
            {
                return sibling;
            }
        }
        else
        {
            return findPreviousSiblingNotSelected(sibling, selectionModel);
        }
    }else{
        //if the parent exist keep searching, else stop the search
        QModelIndex parent = index.model()->parent(index);

        if(parent.isValid())
        {
            return findPreviousSiblingNotSelected(parent, selectionModel);
        }
        else
        {
            return QModelIndex();
        }
    }
}

QModelIndex WBDocumentController::findNextSiblingNotSelected(const QModelIndex &index, QItemSelectionModel *selectionModel)
{
    QModelIndex sibling = index.sibling(index.row() + 1, 0);

    if(sibling.isValid())
    {
        if(!parentIsSelected(sibling, selectionModel)
            && !selectionModel->isSelected(sibling))
        {
            QModelIndex model = mSortFilterProxyModel->mapToSource(sibling);

            if(WBPersistenceManager::persistenceManager()->mDocumentTreeStructureModel->isCatalog(model))
            {
                return findNextSiblingNotSelected(sibling, selectionModel);
            }
            else
            {
                return sibling;
            }
        }
        else
        {
            return findNextSiblingNotSelected(sibling, selectionModel);
        }
    }
    else
    {
        //if the parent exist keep searching, else stop the search
        QModelIndex parent = index.parent();

        if(parent.isValid())
        {
            return findNextSiblingNotSelected(parent, selectionModel);
        }
        else
        {
            return QModelIndex();
        }
    }
}

bool WBDocumentController::parentIsSelected(const QModelIndex& child, QItemSelectionModel *selectionModel)
{
    QModelIndex parent = child.parent();

    while(parent.isValid()){
        if(selectionModel->isSelected(parent)){
            return true;
        }

        parent = parent.parent();
    }

    return false;
}

void WBDocumentController::moveToTrash(QModelIndex &index, WBDocumentTreeModel* docModel)
{
    QModelIndexList list;
    list.push_back(index);
    moveIndexesToTrash(list, docModel);
}

void WBDocumentController::deleteDocumentsInFolderOlderThan(const QModelIndex &index, const int days)
{
    WBDocumentTreeModel *docModel = WBPersistenceManager::persistenceManager()->mDocumentTreeStructureModel;

    QModelIndexList list;
    for (int i = 0; i < docModel->rowCount(index); i++)
    {
        list << docModel->index(i, 0, index);
    }

    foreach (QModelIndex child, list)
    {
        WBDocumentProxy *documentProxy= docModel->proxyForIndex(child);

        if (documentProxy)
        {
            if (documentProxy->lastUpdate().date() < QDateTime::currentDateTime().addDays(-days).date())
            {
                WBPersistenceManager::persistenceManager()->deleteDocument(documentProxy);
            }
        }
        else
        {
            if (docModel->isCatalog(child))
            {
                deleteDocumentsInFolderOlderThan(child, days);
            }
        }
    }
}

void WBDocumentController::deleteEmptyFolders(const QModelIndex &index)
{
    WBDocumentTreeModel *docModel = WBPersistenceManager::persistenceManager()->mDocumentTreeStructureModel;

    QModelIndexList list;
    for (int i = 0; i < docModel->rowCount(index); i++)
    {
        list << docModel->index(i, 0, index);
    }

    if (list.length() > 0)
    {
        foreach (QModelIndex child, list)
        {
            if (docModel->isCatalog(child))
            {
                if (!docModel->containsDocuments(child))
                {
                    deleteIndexAndAssociatedData(child);
                }
            }
        }
    }
    else
    {
        if (docModel->isCatalog(index))
        {
            deleteIndexAndAssociatedData(index);
        }
    }
}

void WBDocumentController::emptyFolder(const QModelIndex &index, DeletionType pDeletionType)
{
    // Issue NC - CFA - 20131029 : ajout d'une popup de confirmation pour la suppression definitive
//    if(pDeletionType == CompleteDelete && !WBApplication::mainWindow->yesNoQuestion(tr("Empty the trash"),tr("You're about to empty the trash.") +"\n\n" + tr("Are you sure ?")))
//        return;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    WBDocumentTreeModel *docModel = WBPersistenceManager::persistenceManager()->mDocumentTreeStructureModel;
    if (!docModel->isCatalog(index)) {
        return;
    }
    while (docModel->rowCount(index)) {
        QModelIndex subIndex = docModel->index(0, 0, index);
        switch (pDeletionType) {
        case MoveToTrash :
            docModel->moveIndex(subIndex, docModel->trashIndex());
            break;

        case CompleteDelete :
            deleteIndexAndAssociatedData(subIndex);
            break;
        default:
            break;
        }

    }

    QApplication::restoreOverrideCursor();
    // Fin issue NC - CFA - 20131029
}

void WBDocumentController::deleteIndexAndAssociatedData(const QModelIndex &pIndex)
{
    WBDocumentTreeModel *docModel = WBPersistenceManager::persistenceManager()->mDocumentTreeStructureModel;
    while (docModel->rowCount(pIndex)) {
        QModelIndex subIndex = docModel->index(0, 0, pIndex);
        deleteIndexAndAssociatedData(subIndex);
    }

    if(pIndex.column() == 0){
        if (docModel->isDocument(pIndex)) {
            WBDocumentProxy *proxyData = docModel->proxyData(pIndex);

            if (proxyData) {
                WBPersistenceManager::persistenceManager()->deleteDocument(proxyData);
            }
        }
    }

    docModel->removeRow(pIndex.row(), pIndex.parent());
}

void WBDocumentController::exportDocument()
{
    QAction *currentExportAction = qobject_cast<QAction *>(sender());
    QVariant actionData = currentExportAction->data();
    WBExportAdaptor* selectedExportAdaptor = WBDocumentManager::documentManager()->supportedExportAdaptors()[actionData.toInt()];

    WBDocumentProxy* proxy = firstSelectedTreeProxy();

    selectedExportAdaptor->persist(proxy);
    emit exportDone();

}

void WBDocumentController::exportDocumentSet()
{

}

void WBDocumentController::documentZoomSliderValueChanged (int value)
{
    mDocumentUI->thumbnailWidget->setThumbnailWidth(value);

    WBSettings::settings()->documentThumbnailWidth->set(value);
}

void WBDocumentController::importFile()
{
    WBDocumentManager *docManager = WBDocumentManager::documentManager();

    QString defaultPath = WBSettings::settings()->lastImportFilePath->get().toString();
    if(defaultPath.isDetached())
        defaultPath = WBSettings::settings()->userDocumentDirectory();
    QString filePath = QFileDialog::getOpenFileName(mParentWidget, tr("Open Supported File"),
                                                    defaultPath, docManager->importFileFilter());

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    QApplication::processEvents();
    QFileInfo fileInfo(filePath);

    if (fileInfo.suffix().toLower() == "ubx") {
        WBPersistenceManager::persistenceManager()->createDocumentProxiesStructure(docManager->importUbx(filePath, WBSettings::userDocumentDirectory()), true);

    } else {
        WBSettings::settings()->lastImportFilePath->set(QVariant(fileInfo.absolutePath()));

        if (filePath.length() > 0)
        {
            WBDocumentProxy* createdDocument = 0;
            QApplication::processEvents();
            QFile selectedFile(filePath);

            WBDocumentTreeModel *docModel = WBPersistenceManager::persistenceManager()->mDocumentTreeStructureModel;

            QModelIndex selectedIndex = firstSelectedTreeIndex();
            QString groupName = "";
            if (selectedIndex.isValid())
            {
                groupName = docModel->isCatalog(selectedIndex)
                    ? docModel->virtualPathForIndex(selectedIndex)
                    : docModel->virtualDirForIndex(selectedIndex);
            }

            showMessage(tr("Importing file %1...").arg(fileInfo.baseName()), true);

            createdDocument = docManager->importFile(selectedFile, groupName);

            if (createdDocument) {
                selectDocument(createdDocument, true, true, true);

            } else {
                showMessage(tr("Failed to import file ... "));
            }
        }
    }

    QApplication::restoreOverrideCursor();

}

void WBDocumentController::addFolderOfImages()
{
    WBDocumentProxy* document = selectedDocumentProxy();

    if (document)
    {
        QString defaultPath = WBSettings::settings()->lastImportFolderPath->get().toString();

        QString imagesDir = QFileDialog::getExistingDirectory(mParentWidget, tr("Import all Images from Folder"), defaultPath);
        QDir parentImageDir(imagesDir);
        parentImageDir.cdUp();

        WBSettings::settings()->lastImportFolderPath->set(QVariant(parentImageDir.absolutePath()));

        if (imagesDir.length() > 0)
        {
            QDir dir(imagesDir);

            int importedImageNumber
                  = WBDocumentManager::documentManager()->addImageDirToDocument(dir, document);

            if (importedImageNumber == 0)
            {
                showMessage(tr("Folder does not contain any image files"));
                WBApplication::applicationController->showDocument();
            }
            else
            {
                document->setMetaData(WBSettings::documentUpdatedAt, WBStringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
                WBMetadataDcSubsetAdaptor::persist(document);
                reloadThumbnails();
            }
        }
    }
}


void WBDocumentController::addFileToDocument()
{
    WBDocumentProxy* document = selectedDocumentProxy();

    if (document)
    {
         addFileToDocument(document);
         reloadThumbnails();
    }
}

WBDocumentProxyTreeItem* WBDocumentController::findDocument(WBDocumentProxy* proxy)
{
	selectDocument(proxy);
	//WBDocumentProxyTreeItem* ppp = dynamic_cast<WBDocumentProxyTreeItem*>(proxy);
	return NULL;
}

bool WBDocumentController::addFileToDocument(WBDocumentProxy* document)
{
    QString defaultPath = WBSettings::settings()->lastImportFilePath->get().toString();
    QString filePath = QFileDialog::getOpenFileName(mParentWidget, tr("Open Supported File"), defaultPath, WBDocumentManager::documentManager()->importFileFilter(true));

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    QApplication::processEvents();

    QFileInfo fileInfo(filePath);
    WBSettings::settings()->lastImportFilePath->set(QVariant(fileInfo.absolutePath()));

    bool success = false;

    if (filePath.length() > 0)
    {
        QApplication::processEvents(); // NOTE: We performed this just a few lines before. Is it really necessary to do it again here??
        QFile selectedFile(filePath);

        showMessage(tr("Importing file %1...").arg(fileInfo.baseName()), true);

        QStringList fileNames;
        fileNames << filePath;
        success = WBDocumentManager::documentManager()->addFilesToDocument(document, fileNames);

        if (success)
        {
            document->setMetaData(WBSettings::documentUpdatedAt, WBStringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
            WBMetadataDcSubsetAdaptor::persist(document);
        }
        else
        {
            showMessage(tr("Failed to import file ... "));
        }
    }

    QApplication::restoreOverrideCursor();

    return success;
}


void WBDocumentController::moveSceneToIndex(WBDocumentProxy* proxy, int source, int target)
{
    if (WBDocumentContainer::movePageToIndex(source, target))
    {
        proxy->setMetaData(WBSettings::documentUpdatedAt, WBStringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
        WBMetadataDcSubsetAdaptor::persist(proxy);

        mDocumentUI->thumbnailWidget->hightlightItem(target);

        mBoardController->setActiveDocumentScene(target);
        mBoardController->reloadThumbnails();
    }
}


void WBDocumentController::thumbnailViewResized()
{
    int maxWidth = qMin(WBSettings::maxThumbnailWidth, mDocumentUI->thumbnailWidget->width());

    mDocumentUI->documentZoomSlider->setMaximum(maxWidth);
}


void WBDocumentController::pageSelectionChanged()
{
    if (mIsClosing)
        return;

    WBDocumentTreeModel *docModel = WBPersistenceManager::persistenceManager()->mDocumentTreeStructureModel;

    bool pageSelected = mDocumentUI->thumbnailWidget->selectedItems().count() > 0;
    bool docSelected = docModel->isDocument(firstSelectedTreeIndex());
    bool folderSelected = docModel->isCatalog(firstSelectedTreeIndex());

    if (pageSelected)
        itemSelectionChanged(Page);
    else if (docSelected)
        itemSelectionChanged(Document);
    else if (folderSelected)
        itemSelectionChanged(Folder);
    else
        itemSelectionChanged(None);

    updateActions();
}

void WBDocumentController::documentSceneChanged(WBDocumentProxy* proxy, int pSceneIndex)
{
    Q_UNUSED(pSceneIndex);

    if (proxy == selectedDocumentProxy())
    {
        reloadThumbnails();
    }

    QModelIndexList sel = mDocumentUI->documentTreeView->selectionModel()->selectedRows(0);

    QModelIndex selection;
    if(sel.count() > 0){
        selection = sel.first();
    }

    TreeViewSelectionChanged(selection, QModelIndex());
}

void WBDocumentController::thumbnailPageDoubleClicked(QGraphicsItem* item, int index)
{
    WBDocumentTreeModel *docModel = WBPersistenceManager::persistenceManager()->mDocumentTreeStructureModel;
    QModelIndex selectedIndex = firstSelectedTreeIndex();

    if (selectedIndex.isValid()) {
        if (docModel->inTrash(selectedIndex)) {
            return;
        }
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    WBSceneThumbnailPixmap* thumb = qgraphicsitem_cast<WBSceneThumbnailPixmap*> (item);

    if (thumb) {
        WBDocumentProxy* proxy = thumb->proxy();
        if (proxy && isOKToOpenDocument(proxy)) {
            mBoardController->setActiveDocumentScene(proxy, index);
            WBApplication::applicationController->showBoard();
        }
    }

    QApplication::restoreOverrideCursor();
}


void WBDocumentController::pageClicked(QGraphicsItem* item, int index)
{
    Q_UNUSED(item);
    Q_UNUSED(index);

    pageSelectionChanged();
}


void WBDocumentController::addToDocument()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    QList<QGraphicsItem*> selectedItems = mDocumentUI->thumbnailWidget->selectedItems();

    if (selectedItems.count() > 0)
    {
        int oldActiveSceneIndex = mBoardController->activeSceneIndex();

        QList<QPair<WBDocumentProxy*, int> > pageInfoList;

        foreach (QGraphicsItem* item, selectedItems)
        {
            WBSceneThumbnailPixmap* thumb = dynamic_cast<WBSceneThumbnailPixmap*> (item);

            if (thumb &&  thumb->proxy())
            {
                QPair<WBDocumentProxy*, int> pageInfo(thumb->proxy(), thumb->sceneIndex());
                pageInfoList << pageInfo;
            }
        }

        for (int i = 0; i < pageInfoList.length(); i++)
        {
            mBoardController->addScene(pageInfoList.at(i).first, pageInfoList.at(i).second, false);
        }

        int newActiveSceneIndex = selectedItems.count() == mBoardController->selectedDocument()->pageCount() ? 0 : oldActiveSceneIndex + 1;
        mDocumentUI->thumbnailWidget->selectItemAt(newActiveSceneIndex, false);
        selectDocument(mBoardController->selectedDocument());
        mBoardController->selectedDocument()->setMetaData(WBSettings::documentUpdatedAt, WBStringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
        WBMetadataDcSubsetAdaptor::persist(mBoardController->selectedDocument());
        mBoardController->reloadThumbnails();

        emit WBApplication::boardController->documentThumbnailsUpdated(this);
        WBApplication::applicationController->showBoard();

        mBoardController->setActiveDocumentScene(newActiveSceneIndex);
    }

    QApplication::restoreOverrideCursor();
}

void WBDocumentController::renameSelectedItem()
{
    if (mDocumentUI->documentTreeView->currentIndex().isValid()) {
        mDocumentUI->documentTreeView->edit(mDocumentUI->documentTreeView->currentIndex());
    }
}

bool WBDocumentController::isOKToOpenDocument(WBDocumentProxy* proxy)
{
    //check version
    QString docVersion = proxy->metaData(WBSettings::documentVersion).toString();

    if (docVersion.isEmpty() || docVersion.startsWith("1.1") || docVersion.startsWith("1.2"))
    {
        return true;
    }
    else
    {
        if (WBApplication::mainWindow->yesNoQuestion(tr("Open Document"),
                tr("The document '%1' has been generated with a newer version of WBoard (%2). By opening it, you may lose some information. Do you want to proceed?")
                    .arg(proxy->metaData(WBSettings::documentName).toString())
                    .arg(docVersion)))
        {
            return true;
        }
        else
        {
            return false;
        }
    }
}


void WBDocumentController::showMessage(const QString& message, bool showSpinningWheel)
{
    if (mMessageWindow)
    {
        int margin = WBSettings::boardMargin;

        QRect newSize = mDocumentUI->thumbnailWidget->geometry();

        #ifdef Q_WS_MACX
            QPoint point(newSize.left() + margin, newSize.bottom() - mMessageWindow->height() - margin);
            mMessageWindow->move(mDocumentUI->thumbnailWidget->mapToGlobal(point));
        #else
            mMessageWindow->move(margin, newSize.height() - mMessageWindow->height() - margin);
        #endif

        mMessageWindow->showMessage(message, showSpinningWheel);
    }
}


void WBDocumentController::hideMessage()
{
    if (mMessageWindow)
        mMessageWindow->hideMessage();
}


void WBDocumentController::addImages()
{
    WBDocumentProxy* document = selectedDocumentProxy();

    if (document)
    {
        QString defaultPath = WBSettings::settings()->lastImportFolderPath->get().toString();

        QString extensions;

        foreach (QString ext, WBSettings::settings()->imageFileExtensions)
        {
            extensions += " *.";
            extensions += ext;
        }

        QStringList images = QFileDialog::getOpenFileNames(mParentWidget, tr("Add all Images to Document"),
                defaultPath, tr("All Images (%1)").arg(extensions));

        if (images.length() > 0)
        {
            QFileInfo firstImage(images.at(0));

            WBSettings::settings()->lastImportFolderPath->set(QVariant(firstImage.absoluteDir().absolutePath()));

            int importedImageNumber
                = WBDocumentManager::documentManager()->addFilesToDocument(document, images);

            if (importedImageNumber == 0)
            {
                WBApplication::showMessage(tr("Selection does not contain any image files!"));
                WBApplication::applicationController->showDocument();
            }
            else
            {
                document->setMetaData(WBSettings::documentUpdatedAt, WBStringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
                WBMetadataDcSubsetAdaptor::persist(document);
                reloadThumbnails();
            }
        }
    }
}

void WBDocumentController::toggleDocumentToolsPalette()
{
    if (!mToolsPalette->isVisible() && !mToolsPalettePositionned)
    {
        mToolsPalette->adjustSizeAndPosition();
        int left = controlView()->width() - 20 - mToolsPalette->width();
        int top = (controlView()->height() - mToolsPalette->height()) / 2;

        mToolsPalette->setCustomPosition(true);
        mToolsPalette->move(left, top);

        mToolsPalettePositionned = true;
    }

    bool visible = mToolsPalette->isVisible();
    mToolsPalette->setVisible(!visible);
}


void WBDocumentController::cut()
{
    // TODO - implemented me
}


void WBDocumentController::copy()
{
    // TODO - implemented me
}


void WBDocumentController::paste()
{
    // TODO - implemented me
}


void WBDocumentController::focusChanged(QWidget *old, QWidget *current)
{
    WBDocumentTreeModel *treeModel = WBPersistenceManager::persistenceManager()->mDocumentTreeStructureModel;

    if (current == mDocumentUI->thumbnailWidget)
    {
        if (mDocumentUI->thumbnailWidget->selectedItems().count() > 0)
            mSelectionType = Page;
        else
            mSelectionType = None;
    }
    else if (current == mDocumentUI->documentTreeView)
    {
        if (treeModel->isDocument(firstSelectedTreeIndex()))
            mSelectionType = Document;
        else if (treeModel->isCatalog(firstSelectedTreeIndex()))
            mSelectionType = Folder;
        else
            mSelectionType = None;
    }
    else if (current == mDocumentUI->documentZoomSlider)
    {
        if (mDocumentUI->thumbnailWidget->selectedItems().count() > 0)
            mSelectionType = Page;
        else
            mSelectionType = None;
    }
    else
    {
        if (old != mDocumentUI->thumbnailWidget &&
            old != mDocumentUI->documentTreeView &&
            old != mDocumentUI->documentZoomSlider)
        {
            if (current && (current->metaObject()->className() != QPushButton::staticMetaObject.className()))
                mSelectionType = None;
        }
    }
}

void WBDocumentController::updateActions()
{
    if (mIsClosing)
        return;

    WBDocumentTreeModel *docModel = WBPersistenceManager::persistenceManager()->mDocumentTreeStructureModel;

    QModelIndexList list = mDocumentUI->documentTreeView->selectionModel()->selectedRows(0);

    //if in multi selection, juste activate the actionDelete
    if(list.count() > 1){
        mMainWindow->actionNewDocument->setEnabled(false);
        mMainWindow->actionNewFolder->setEnabled(false);
        mMainWindow->actionExport->setEnabled(false);
        mMainWindow->actionDuplicate->setEnabled(false);
        mMainWindow->actionOpen->setEnabled(false);
        mMainWindow->actionRename->setEnabled(false);

        mMainWindow->actionAddToWorkingDocument->setEnabled(false);
        mMainWindow->actionDocumentAdd->setEnabled(false);
        mMainWindow->actionImport->setEnabled(false);
        mMainWindow->actionDelete->setEnabled(true);

        return;
    }

#ifdef Q_OS_OSX
        mMainWindow->actionDelete->setShortcut(QKeySequence(Qt::Key_Backspace));
#else
        mMainWindow->actionDelete->setShortcut(QKeySequence(Qt::Key_Delete));
#endif

    QModelIndex selectedIndex = firstSelectedTreeIndex();
    WBDocumentProxy *selectedProxy = docModel->proxyData(selectedIndex);
    int pageCount = -1;
    if (selectedProxy) {
        pageCount = selectedProxy->pageCount();
    }

    bool pageSelected = false;
    bool groupSelected = false;
    bool docSelected = false;

    if (mSelectionType == Page) {
        pageSelected = true;
    } else {
        if (docModel->isDocument(firstSelectedTreeIndex())) {
            docSelected = true;
        } else if (docModel->isCatalog(firstSelectedTreeIndex())) {
            groupSelected = true;
        }
    }

    bool trashSelected = docModel->inTrash(selectedIndex) || selectedIndex == docModel->trashIndex()  ? true : false;

    mMainWindow->actionNewDocument->setEnabled(docModel->newNodeAllowed(selectedIndex));
    mMainWindow->actionNewFolder->setEnabled(docModel->newNodeAllowed(selectedIndex));
    mMainWindow->actionExport->setEnabled((docSelected || pageSelected || groupSelected) && !trashSelected);
    updateExportSubActions(selectedIndex);

    bool firstSceneSelected = false;

    if (docSelected) {
        mMainWindow->actionDuplicate->setEnabled(!trashSelected);

    } else if (pageSelected) {
        QList<QGraphicsItem*> selection = mDocumentUI->thumbnailWidget->selectedItems();
        if(pageCount == 1) {
            mMainWindow->actionDuplicate->setEnabled(!trashSelected && pageCanBeDuplicated(WBDocumentContainer::pageFromSceneIndex(0)));

        } else {
            for (int i = 0; i < selection.count() && !firstSceneSelected; i += 1) {
                if (qgraphicsitem_cast<WBSceneThumbnailPixmap*>(selection.at(i))->sceneIndex() == 0) {
                    mMainWindow->actionDuplicate->setEnabled(!trashSelected && pageCanBeDuplicated(WBDocumentContainer::pageFromSceneIndex(0)));
                    firstSceneSelected = true;
                    break;
                }
            }
            if (!firstSceneSelected) {
                mMainWindow->actionDuplicate->setEnabled(!trashSelected);
            }
        }

    } else {
        mMainWindow->actionDuplicate->setEnabled(false);
    }

    mMainWindow->actionOpen->setEnabled((docSelected || pageSelected) && !trashSelected);
    mMainWindow->actionRename->setEnabled(docModel->isOkToRename(selectedIndex));

    mMainWindow->actionAddToWorkingDocument->setEnabled(pageSelected
            && !(selectedProxy == mBoardController->selectedDocument()) && !trashSelected);

    DeletionType deletionForSelection = deletionTypeForSelection(mSelectionType, selectedIndex, docModel);
    mMainWindow->actionDelete->setEnabled(deletionForSelection != NoDeletion);

    switch (static_cast<int>(deletionForSelection)) {
    case MoveToTrash :
    case DeletePage :
        mMainWindow->actionDelete->setIcon(QIcon(":/images/trash.png"));
        mMainWindow->actionDelete->setText(tr("Trash"));
        break;
    case CompleteDelete :
        mMainWindow->actionDelete->setIcon(QIcon(":/images/toolbar/deleteDocument.png"));
        mMainWindow->actionDelete->setText(tr("Delete"));
        break;
    case EmptyFolder :
        mMainWindow->actionDelete->setIcon(QIcon(":/images/trash.png"));
        mMainWindow->actionDelete->setText(tr("Empty"));
        break;
    case EmptyTrash :
        mMainWindow->actionDelete->setIcon(QIcon(":/images/toolbar/deleteDocument.png"));
        mMainWindow->actionDelete->setText(tr("Empty"));
        break;
    }

    mMainWindow->actionDocumentAdd->setEnabled((docSelected || pageSelected) && !trashSelected);
    mMainWindow->actionImport->setEnabled(!trashSelected);

}

void WBDocumentController::updateExportSubActions(const QModelIndex &selectedIndex)
{
    WBDocumentManager *documentManager = WBDocumentManager::documentManager();
    for (int i = 0; i < documentManager->supportedExportAdaptors().length(); i++)
    {
        WBExportAdaptor* adaptor = documentManager->supportedExportAdaptors()[i];
        if (adaptor->associatedAction()) {
            adaptor->associatedAction()->setEnabled(adaptor->associatedActionactionAvailableFor(selectedIndex));
        }
    }
}

void WBDocumentController::currentIndexMoved(const QModelIndex &newIndex, const QModelIndex &PreviousIndex)
{
    Q_UNUSED(newIndex);
    Q_UNUSED(PreviousIndex);

    WBDocumentTreeModel *docModel = WBPersistenceManager::persistenceManager()->mDocumentTreeStructureModel;
    WBDocumentProxy *newProxy = docModel->proxyData(newIndex);
    if (newProxy) {
        WBDocumentProxy *cp = new WBDocumentProxy(*newProxy); // we cannot use newProxy because it will be destroyed later
        pureSetDocument(cp);
        mBoardController->pureSetDocument(cp);
        mBoardController->pureSetDocument(newProxy);
    }
    mCurrentIndexMoved = true;
}

void WBDocumentController::deletePages(QList<QGraphicsItem *> itemsToDelete)
{
    if (itemsToDelete.count() > 0)
    {
        QList<int> sceneIndexes;
        WBDocumentProxy* proxy = 0;

        foreach (QGraphicsItem* item, itemsToDelete)
        {
            WBSceneThumbnailPixmap* thumb = dynamic_cast<WBSceneThumbnailPixmap*> (item);

            if (thumb)
            {
                proxy = thumb->proxy();
                if (proxy)
                {
                    sceneIndexes.append(thumb->sceneIndex());
                }

            }
        }
        WBDocumentContainer::deletePages(sceneIndexes);
        emit WBApplication::boardController->documentThumbnailsUpdated(this);

        proxy->setMetaData(WBSettings::documentUpdatedAt, WBStringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
        WBMetadataDcSubsetAdaptor::persist(proxy);

        int minIndex = proxy->pageCount() - 1;
        foreach (int i, sceneIndexes)
             minIndex = qMin(i, minIndex);

        if (mBoardController->activeSceneIndex() > minIndex)
        {
            mBoardController->setActiveSceneIndex(minIndex);
        }

        mDocumentUI->thumbnailWidget->selectItemAt(minIndex);

        mBoardController->setActiveDocumentScene(minIndex);
        mBoardController->reloadThumbnails();
    }
}

int WBDocumentController::getSelectedItemIndex()
{
    QList<QGraphicsItem*> selectedItems = mDocumentUI->thumbnailWidget->selectedItems();

    if (selectedItems.count() > 0)
    {
        WBSceneThumbnailPixmap* thumb = dynamic_cast<WBSceneThumbnailPixmap*> (selectedItems.last());
        return thumb->sceneIndex();
    }
    else return -1;
}

bool WBDocumentController::pageCanBeMovedUp(int page)
{
    return page >= 1;
}

bool WBDocumentController::pageCanBeMovedDown(int page)
{
    return page < selectedDocument()->pageCount() - 1;
}

bool WBDocumentController::pageCanBeDuplicated(int page)
{
    return page != 0;
}

bool WBDocumentController::pageCanBeDeleted(int page)
{
    return page != 0;
}

void WBDocumentController::setDocument(WBDocumentProxy *document, bool forceReload)
{
    WBDocumentContainer::setDocument(document, forceReload);
}

QModelIndex WBDocumentController::firstSelectedTreeIndex()
{
    return selectedTreeIndexes().count() ? selectedTreeIndexes().first() : QModelIndex();
}

WBDocumentController::DeletionType
WBDocumentController::deletionTypeForSelection(LastSelectedElementType pTypeSelection
                                               , const QModelIndex &selectedIndex
                                               , WBDocumentTreeModel *docModel) const
{

    if (pTypeSelection == Page) {
        if (!firstAndOnlySceneSelected()) {
            return DeletePage;
        }
    } else if (docModel->isConstant(selectedIndex)) {
        if (selectedIndex == docModel->trashIndex()) {
            if (docModel->rowCount(selectedIndex) > 0)
                return EmptyTrash;
            else
                return NoDeletion;
        }

        if (selectedIndex.isValid())
            return EmptyFolder;
        else
            return NoDeletion;
    } else if (pTypeSelection != None) {
        if (docModel->inTrash(selectedIndex)) {
            return CompleteDelete;
        } else {
            return MoveToTrash;
        }
    }

    return NoDeletion;
}

bool WBDocumentController::firstAndOnlySceneSelected() const
{
    bool firstSceneSelected = false;
    QList<QGraphicsItem*> selection = mDocumentUI->thumbnailWidget->selectedItems();
    for(int i = 0; i < selection.count() && !firstSceneSelected; i += 1)
    {
        WBSceneThumbnailPixmap* p = dynamic_cast<WBSceneThumbnailPixmap*>(selection.at(i));
        if (p)
        {
            int pageCount = p->proxy()->pageCount();
            if (pageCount > 1) //not the only scene
            {
                return false;
            }
            else
            {
                if (p->sceneIndex() == 0)
                {
                    return true; //the first and only scene
                }
            }
        }
    }

    return false;
}

void WBDocumentController:: refreshDocumentThumbnailsView(WBDocumentContainer*)
{
    WBDocumentTreeModel *docModel = WBPersistenceManager::persistenceManager()->mDocumentTreeStructureModel;
    WBDocumentProxy *currentDocumentProxy = selectedDocument();

    QModelIndex current = docModel->indexForProxy(currentDocumentProxy);

    if (!current.isValid())
    {
        mDocumentUI->thumbnailWidget->setGraphicsItems(QList<QGraphicsItem*>()
                                                       , QList<QUrl>()
                                                       , QStringList()
                                                       , WBApplication::mimeTypeUniboardPage);
        return;
    }

    QList<const QPixmap*> thumbs;

    if (currentDocumentProxy)
    {
        WBThumbnailAdaptor::load(currentDocumentProxy, thumbs);
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    QList<QGraphicsItem*> items;
    QList<QUrl> itemsPath;

    QGraphicsPixmapItem *selection = 0;

    QStringList labels;

    if (currentDocumentProxy)
    {
        for (int i = 0; i < currentDocumentProxy->pageCount(); i++)
        {
            const QPixmap* pix = thumbs.at(i);
            QGraphicsPixmapItem *pixmapItem = new WBSceneThumbnailPixmap(*pix, currentDocumentProxy, i); // deleted by the tree widget

            if (currentDocumentProxy == mBoardController->selectedDocument() && mBoardController->activeSceneIndex() == i)
            {
                selection = pixmapItem;
            }

            items << pixmapItem;
            int pageIndex = pageFromSceneIndex(i);
            if(pageIndex)
                labels << tr("Page %1").arg(pageIndex);
            else
                labels << tr("Title page");

            itemsPath.append(QUrl::fromLocalFile(currentDocumentProxy->persistencePath() + QString("/pages/%1").arg(WBDocumentContainer::pageFromSceneIndex(i))));
        }
    }

    mDocumentUI->thumbnailWidget->setGraphicsItems(items, itemsPath, labels, WBApplication::mimeTypeUniboardPage);

    if (docModel->inTrash(current)) {
        mDocumentUI->thumbnailWidget->setDragEnabled(false);
    } else {
        mDocumentUI->thumbnailWidget->setDragEnabled(true);
    }

    mDocumentUI->thumbnailWidget->ensureVisible(0, 0, 10, 10);

    if (selection)
    {
        disconnect(mDocumentUI->thumbnailWidget->scene(), SIGNAL(selectionChanged()), this, SLOT(pageSelectionChanged()));
        WBSceneThumbnailPixmap *currentScene = dynamic_cast<WBSceneThumbnailPixmap*>(selection);
        if (currentScene)
            mDocumentUI->thumbnailWidget->hightlightItem(currentScene->sceneIndex());
        connect(mDocumentUI->thumbnailWidget->scene(), SIGNAL(selectionChanged()), this, SLOT(pageSelectionChanged()));
    }

    QApplication::restoreOverrideCursor();
}

void WBDocumentController::createNewDocumentInUntitledFolder()
{
    WBPersistenceManager *pManager = WBPersistenceManager::persistenceManager();
    WBDocumentTreeModel *docModel = pManager->mDocumentTreeStructureModel;

    QString groupName = docModel->virtualPathForIndex(docModel->untitledDocumentsIndex());

    WBDocumentProxy *document = pManager->createDocument(groupName);
    selectDocument(document);

    if (document)
        pManager->mDocumentTreeStructureModel->markDocumentAsNew(document);
}

void WBDocumentController::collapseAll()
{
    //disable the animations because the view port will be in a invalid state
    mDocumentUI->documentTreeView->setAnimated(false);

    mDocumentUI->documentTreeView->collapseAll();

    QPersistentModelIndex untiltedDocumentIndex = WBPersistenceManager::persistenceManager()->mDocumentTreeStructureModel->untitledDocumentsIndex();
    QPersistentModelIndex myDocumentIndex = WBPersistenceManager::persistenceManager()->mDocumentTreeStructureModel->myDocumentsIndex();

    WBSortFilterProxyModel *proxy = dynamic_cast<WBSortFilterProxyModel*>(mDocumentUI->documentTreeView->model());

    if(proxy){
        mDocumentUI->documentTreeView->setExpanded(proxy->mapFromSource(myDocumentIndex), true);
        mDocumentUI->documentTreeView->setExpanded(proxy->mapFromSource(untiltedDocumentIndex), true);
    }else{
        mDocumentUI->documentTreeView->setExpanded(myDocumentIndex, true);
        mDocumentUI->documentTreeView->setExpanded(untiltedDocumentIndex, true);
    }

    mDocumentUI->documentTreeView->setAnimated(true);
}

void WBDocumentController::expandAll()
{
    mDocumentUI->documentTreeView->setAnimated(false);

    mDocumentUI->documentTreeView->expandAll();

    mDocumentUI->documentTreeView->setAnimated(true);
}
