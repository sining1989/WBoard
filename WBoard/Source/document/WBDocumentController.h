#ifndef WBDOCUMENTCONTROLLER_H_
#define WBDOCUMENTCONTROLLER_H_

#include <QtWidgets>
#include "document/WBDocumentContainer.h"
#include "core/WBApplicationController.h"
#include "core/WBApplication.h"
#include "document/WBSortFilterProxyModel.h"

namespace Ui
{
    class documents;
}

#include "gui/WBMessageWindow.h"

class WBGraphicsScene;
class QDialog;
class WBDocumentProxy;
class WBBoardController;
class WBThumbnailsScene;
class WBDocumentGroupTreeItem;
class WBDocumentProxyTreeItem;
class WBMainWindow;
class WBDocumentToolsPalette;

class WBDocumentReplaceDialog : public QDialog
{
    Q_OBJECT

public:
    WBDocumentReplaceDialog(const QString &pIncommingName, const QStringList &pFileList, QWidget *parent = 0, Qt::WindowFlags pFlags = 0);
    void setRegexp(const QRegExp pRegExp);
    bool validString(const QString &pStr);
    void setFileNameAndList(const QString &fileName, const QStringList &pLst);
    QString  labelTextWithName(const QString &documentName) const;
    QString lineEditText() const {return mLineEdit->text();}

signals:
    void createNewFolder(QString str);
    void closeDialog();

private slots:
    void accept();
    void reject();

    void reactOnTextChanged(const QString &pStr);

private:
    QLineEdit *mLineEdit;
    QRegExpValidator *mValidator;
    QStringList mFileNameList;
    QString mIncommingName;
    QPushButton *acceptButton;
    const QString acceptText;
    const QString replaceText;
    const QString cancelText;
    QLabel *mLabelText;
};


class WBDocumentTreeNode
{
public:
    friend class WBDocumentTreeModel;

    enum Type {
        Catalog = 0
        , Document
    };

    WBDocumentTreeNode(Type pType, const QString &pName, const QString &pDisplayName = QString(), WBDocumentProxy *pProxy = 0);
    WBDocumentTreeNode() : mType(Catalog), mParent(0), mProxy(0) {;}
    ~WBDocumentTreeNode();

    QList<WBDocumentTreeNode*> children() const {return mChildren;}
    WBDocumentTreeNode *parentNode() {return mParent;}
    Type nodeType() const {return mType;}
    QString nodeName() const {return mName;}
    QString displayName() const {return mDisplayName;}
    void setNodeName(const QString &str) {mName = str; mDisplayName = str;}
    void addChild(WBDocumentTreeNode *pChild);
    void insertChild(int pIndex, WBDocumentTreeNode *pChild);
    void moveChild(WBDocumentTreeNode *child, int index, WBDocumentTreeNode *newParent);
    void removeChild(int index);
    WBDocumentProxy *proxyData() const {return mProxy;}
    bool isRoot() {return !mParent;}
    bool isTopLevel()
    {
        if (mParent)
        {
            return !mParent->mParent;
        }
        else
			return false;
    }
    WBDocumentTreeNode *clone();
    QString dirPathInHierarchy();

    bool findNode(WBDocumentTreeNode *node);
    WBDocumentTreeNode *nextSibling();
    WBDocumentTreeNode *previousSibling();

private:
    Type mType;
    QString mName;
    QString mDisplayName;
    WBDocumentTreeNode *mParent;
    QList<WBDocumentTreeNode*> mChildren;
    QPointer<WBDocumentProxy> mProxy;
};
Q_DECLARE_METATYPE(WBDocumentTreeNode*)

class WBDocumentTreeModel : public QAbstractItemModel {
    Q_OBJECT

public:
    enum eAncestors {
        aMyDocuments
        , aUntitledDocuments
        , aModel
        , aTrash
    };

    enum eCopyMode {
        aReference
        , aContentCopy
    };

    enum eAddItemMode {
        aEnd = 0         
        , aDetectPosition 
    };

    enum eDocumentData{
        DataNode = Qt::UserRole +1,
        CreationDate,
        UpdateDate
    };

    WBDocumentTreeModel(QObject *parent = 0);
    ~WBDocumentTreeModel();

    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    Qt::ItemFlags flags ( const QModelIndex & index ) const;
    Qt::DropActions supportedDropActions() const {return Qt::MoveAction | Qt::CopyAction;}
    QStringList mimeTypes() const;
    QMimeData *mimeData (const QModelIndexList &indexes) const;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
    bool removeRows(int row, int count, const QModelIndex &parent);

    bool containsDocuments(const QModelIndex& index);

    QModelIndex indexForNode(WBDocumentTreeNode *pNode) const;
    QPersistentModelIndex persistentIndexForNode(WBDocumentTreeNode *pNode);
//    bool insertRow(int row, const QModelIndex &parent);

    QPersistentModelIndex copyIndexToNewParent(const QModelIndex &source, const QModelIndex &newParent, eCopyMode pMode = aReference);

    void copyIndexToNewParent(const QModelIndexList &list, const QModelIndex &newParent, eCopyMode pMode);

    void moveIndex(const QModelIndex &what, const QModelIndex &destination);
    WBDocumentTreeNode *currentNode() const {return mCurrentNode;} //work around for sorting model.
    void setCurrentNode(WBDocumentTreeNode *pNode) {mCurrentNode = pNode;}
    QModelIndex currentIndex() {return indexForNode(mCurrentNode);} //index representing a current document
    QModelIndex indexForProxy(WBDocumentProxy *pSearch) const;
    void setCurrentDocument(WBDocumentProxy *pDocument);
    void setRootNode(WBDocumentTreeNode *pRoot);
    WBDocumentTreeNode *rootNode() const {return mRootNode;}
    WBDocumentProxy *proxyForIndex(const QModelIndex &pIndex) const;
    QString virtualDirForIndex(const QModelIndex &pIndex) const;
    QString virtualPathForIndex(const QModelIndex &pIndex) const;
    QStringList nodeNameList(const QModelIndex &pIndex) const;
    bool newNodeAllowed(const QModelIndex &pSelectedIndex)  const;
    QModelIndex goTo(const QString &dir);
    bool inTrash(const QModelIndex &index) const;
    bool inUntitledDocuments(const QModelIndex &index) const;
    bool isCatalog(const QModelIndex &index) const {return nodeFromIndex(index)->nodeType() == WBDocumentTreeNode::Catalog;}
    bool isDocument(const QModelIndex &index) const {return nodeFromIndex(index)->nodeType() == WBDocumentTreeNode::Document;}
    bool isToplevel(const QModelIndex &index) const {return nodeFromIndex(index) ? nodeFromIndex(index)->isTopLevel() : false;}
    bool isConstant(const QModelIndex &index) const {return isToplevel(index) || (index == mUntitledDocuments);}
    bool isOkToRename(const QModelIndex &index) const {return flags(index) & Qt::ItemIsEditable;}
    WBDocumentProxy *proxyData(const QModelIndex &index) const {return nodeFromIndex(index)->proxyData();}
    void addDocument(WBDocumentProxy *pProxyData, const QModelIndex &pParent = QModelIndex());
    void addNewDocument(WBDocumentProxy *pProxyData, const QModelIndex &pParent = QModelIndex());
    QModelIndex addCatalog(const QString &pName, const QModelIndex &pParent);
    QList<WBDocumentProxy*> newDocuments() {return mNewDocuments;}
    void markDocumentAsNew(WBDocumentProxy *pDoc) {if (indexForProxy(pDoc).isValid()) mNewDocuments << pDoc;}
    void setNewName(const QModelIndex &index, const QString &newName);
    QString adjustNameForParentIndex(const QString &pName, const QModelIndex &pIndex);

    QPersistentModelIndex myDocumentsIndex() const {return mMyDocuments;}
    QPersistentModelIndex trashIndex() const {return mTrash;}
    QPersistentModelIndex untitledDocumentsIndex() const {return mMyDocuments;}
    WBDocumentTreeNode *nodeFromIndex(const QModelIndex &pIndex) const;
    static bool nodeLessThan(const WBDocumentTreeNode *firstIndex, const WBDocumentTreeNode *secondIndex);
    void setHighLighted(const QModelIndex &newHighLighted) {mHighLighted = newHighLighted;}
    QModelIndex highLighted() {return mHighLighted;}

    bool ascendingOrder() const{ return mAscendingOrder; }
    QDateTime findNodeDate(WBDocumentTreeNode *node, QString type) const;
    bool inMyDocuments(const QModelIndex &index) const;
    void moveIndexes(const QModelIndexList &source, const QModelIndex &destination);
    bool isDescendantOf(const QModelIndex &pPossibleDescendant, const QModelIndex &pPossibleAncestor) const;

signals:
    void indexChanged(const QModelIndex &newIndex, const QModelIndex &oldIndex);
    void currentIndexMoved(const QModelIndex &newIndex, const QModelIndex &previous); 

private:
    WBDocumentTreeNode *mRootNode;
    WBDocumentTreeNode *mCurrentNode;

    WBDocumentTreeNode *findProxy(WBDocumentProxy *pSearch, WBDocumentTreeNode *pParent) const;
    QModelIndex pIndexForNode(const QModelIndex &parent, WBDocumentTreeNode *pNode) const;
    QModelIndex addNode(WBDocumentTreeNode *pFreeNode, const QModelIndex &pParent, eAddItemMode pMode = aDetectPosition);
    int positionForParent(WBDocumentTreeNode *pFreeNode, WBDocumentTreeNode *pParentNode);
    void fixNodeName(const QModelIndex &source, const QModelIndex &dest);
    void updateIndexNameBindings(WBDocumentTreeNode *nd);
    QPersistentModelIndex mRoot;
    QPersistentModelIndex mMyDocuments;
    QPersistentModelIndex mTrash;
    QPersistentModelIndex mUntitledDocuments;
    QList<WBDocumentProxy*> mNewDocuments;
    QModelIndex mHighLighted;

    bool mAscendingOrder;

    QDateTime findCatalogUpdatedDate(WBDocumentTreeNode *node) const;
    QDateTime findCatalogCreationDate(WBDocumentTreeNode *node) const;
};

class WBDocumentTreeMimeData : public QMimeData
{
    Q_OBJECT

    public:
        QList<QModelIndex> indexes() const {return mIndexes;}
        void setIndexes (const QList<QModelIndex> &pIndexes) {mIndexes = pIndexes;}

    private:
        QList<QModelIndex> mIndexes;
};

class WBDocumentTreeView : public QTreeView
{
    Q_OBJECT

public:
    WBDocumentTreeView (QWidget *parent = 0);

    QModelIndex mapIndexToSource(const QModelIndex &index);
    QModelIndexList mapIndexesToSource(const QModelIndexList &indexes);

public slots:
    void setSelectedAndExpanded(const QModelIndex &pIndex, bool pExpand = true, bool pEdit = false);
    void onModelIndexChanged(const QModelIndex &pNewIndex, const QModelIndex &pOldIndex);
    void hSliderRangeChanged(int min, int max);

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);
    void paintEvent(QPaintEvent *event);

    WBDocumentTreeModel *fullModel() {return qobject_cast<WBDocumentTreeModel*>(model());}
    void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);

private:
    bool isAcceptable(const QModelIndex &dragIndex, const QModelIndex &atIndex);
    Qt::DropAction acceptableAction(const QModelIndex &dragIndex, const QModelIndex &atIndex);
    void updateIndexEnvirons(const QModelIndex &index);
};

class WBDocumentTreeItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    WBDocumentTreeItemDelegate(QObject *parent = 0);

private slots:
    void commitAndCloseEditor();
    void processChangedText(const QString &str) const;
    bool validateString(const QString &str) const;

protected:
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex &index) const;

private:
    mutable QStringList mExistingFileNames;
};

class WBDocumentController : public WBDocumentContainer
{
    Q_OBJECT

public:
    enum DeletionType {
        MoveToTrash = 0
        , CompleteDelete
        , EmptyFolder
        , EmptyTrash
        , DeletePage
        , NoDeletion
    };

    enum LastSelectedElementType
    {
        None = 0, Folder, Document, Page
    };

    enum SortOrder
    {
        ASC = 0,
        DESC
    };

    enum SortKind
    {
        CreationDate,
        UpdateDate,
        Alphabetical
    };

    WBDocumentController(WBMainWindow* mainWindow);
    virtual ~WBDocumentController();

    void closing();
    QWidget* controlView();
    WBDocumentProxyTreeItem* findDocument(WBDocumentProxy* proxy);
    bool addFileToDocument(WBDocumentProxy* document);
    void deletePages(QList<QGraphicsItem*> itemsToDelete);
    int getSelectedItemIndex();

    bool pageCanBeMovedUp(int page);
    bool pageCanBeMovedDown(int page);
    bool pageCanBeDuplicated(int page);
    bool pageCanBeDeleted(int page);
    QString documentTrashGroupName(){ return mDocumentTrashGroupName;}
    QString defaultDocumentGroupName(){ return mDefaultDocumentGroupName;}

    void setDocument(WBDocumentProxy *document, bool forceReload = false);
    QModelIndex firstSelectedTreeIndex();
    WBDocumentProxy *firstSelectedTreeProxy();
    inline DeletionType deletionTypeForSelection(LastSelectedElementType pTypeSelection
                                                    , const QModelIndex &selectedIndex
                                                    , WBDocumentTreeModel *docModel) const;
    bool firstAndOnlySceneSelected() const;
    QWidget *mainWidget() const {return mDocumentWidget;}

    void moveToTrash(QModelIndex &index, WBDocumentTreeModel* docModel);

    void deleteDocumentsInFolderOlderThan(const QModelIndex &index, const int days);
    void deleteEmptyFolders(const QModelIndex &index);

    QModelIndex mapIndexToSource(const QModelIndex &index);
    QModelIndexList mapIndexesToSource(const QModelIndexList &indexes);

    void sortDocuments(int kind, int order);

    void moveIndexesToTrash(const QModelIndexList &list, WBDocumentTreeModel *docModel);
    QModelIndex findPreviousSiblingNotSelected(const QModelIndex &index, QItemSelectionModel *selectionModel);
    QModelIndex findNextSiblingNotSelected(const QModelIndex &index, QItemSelectionModel *selectionModel);
    bool parentIsSelected(const QModelIndex& child, QItemSelectionModel *selectionModel);

signals:
    void exportDone();
    void reorderDocumentsRequested();

public slots:
    void createNewDocument();
    void refreshDateColumns();
    void reorderDocuments();
    void createNewDocumentInUntitledFolder();

    void createNewDocumentGroup();
    void deleteSelectedItem();
    void deleteSingleItem(QModelIndex index, WBDocumentTreeModel *docModel);
    void deleteMultipleItems(QModelIndexList indexes, WBDocumentTreeModel *docModel);
    void emptyFolder(const QModelIndex &index, DeletionType pDeletionType = MoveToTrash);
    void deleteIndexAndAssociatedData(const QModelIndex &pIndex);
    void renameSelectedItem();
    void openSelectedItem();
    void duplicateSelectedItem();
    void importFile();
    void moveSceneToIndex(WBDocumentProxy* proxy, int source, int target);
    void selectDocument(WBDocumentProxy* proxy, bool setAsCurrentDocument = true, const bool onImport = false, const bool editMode = false);
    void show();
    void hide();
    void showMessage(const QString& message, bool showSpinningWheel = false);
    void hideMessage();
    void toggleDocumentToolsPalette();
    void cut();
    void copy();
    void paste();
    void focusChanged(QWidget *old, QWidget *current);
    void updateActions();
    void updateExportSubActions(const QModelIndex &selectedIndex);
    void currentIndexMoved(const QModelIndex &newIndex, const QModelIndex &PreviousIndex);

    void onSortKindChanged(int index);
    void onSortOrderChanged(bool order);
    void onSplitterMoved(int size, int index);
    void collapseAll();
    void expandAll();

protected:
    virtual void setupViews();
    virtual void setupToolbar();
    void setupPalettes();
    bool isOKToOpenDocument(WBDocumentProxy* proxy);
    WBDocumentProxy* selectedDocumentProxy();
    QList<WBDocumentProxy*> selectedProxies();
    QModelIndexList selectedTreeIndexes();
    WBDocumentProxyTreeItem* selectedDocumentProxyTreeItem();
    WBDocumentGroupTreeItem* selectedDocumentGroupTreeItem();
    QStringList allGroupNames();
	void moveDocumentToTrash(WBDocumentGroupTreeItem* groupTi, WBDocumentProxyTreeItem *proxyTi);
	void moveFolderToTrash(WBDocumentGroupTreeItem* groupTi);
    LastSelectedElementType mSelectionType;

private:
    QWidget *mParentWidget;
    WBBoardController *mBoardController;
    Ui::documents* mDocumentUI;
    WBMainWindow* mMainWindow;
    QWidget *mDocumentWidget;
    QPointer<WBMessageWindow> mMessageWindow;
    QAction* mAddFolderOfImagesAction;
    QAction* mAddFileToDocumentAction;
    QAction* mAddImagesAction;
    bool mIsClosing;
    WBDocumentToolsPalette *mToolsPalette;
    bool mToolsPalettePositionned;

    QString mDocumentTrashGroupName;
    QString mDefaultDocumentGroupName;

    bool mCurrentIndexMoved;

    WBSortFilterProxyModel *mSortFilterProxyModel;

public slots:
    void TreeViewSelectionChanged(const QModelIndex &current, const QModelIndex &previous);
    void TreeViewSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

private slots:
    void documentZoomSliderValueChanged (int value);
    void itemSelectionChanged(LastSelectedElementType newSelection);
    void exportDocument();
    void exportDocumentSet();

    void thumbnailViewResized();
    void pageSelectionChanged();

    void documentSceneChanged(WBDocumentProxy* proxy, int pSceneIndex);

    void thumbnailPageDoubleClicked(QGraphicsItem* item, int index);
    void pageClicked(QGraphicsItem* item, int index);
    void addToDocument();

    void addFolderOfImages();
    void addFileToDocument();
    void addImages();
    void refreshDocumentThumbnailsView(WBDocumentContainer* source);
};
#endif /* WBDOCUMENTCONTROLLER_H_ */
