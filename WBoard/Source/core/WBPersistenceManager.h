#ifndef WBPERSISTENCEMANAGER_H_
#define WBPERSISTENCEMANAGER_H_

#include <QtCore>

#include "WBSceneCache.h"

class QDomNode;
class QDomElement;
class WBDocument;
class WBDocumentProxy;
class WBGraphicsScene;
class WBDocumentTreeNode;
class WBDocumentTreeModel;

class WBPersistenceManager : public QObject
{
    Q_OBJECT

    private:
        WBPersistenceManager(QObject *pParent = 0);
        static WBPersistenceManager* sSingleton;

    public:
        virtual ~WBPersistenceManager();

        static const QString imageDirectory;
        static const QString objectDirectory;
        static const QString videoDirectory;
        static const QString audioDirectory;
        static const QString widgetDirectory;
        static const QString fileDirectory;

        static const QString myDocumentsName;
        static const QString modelsName;
        static const QString untitledDocumentsName;
        static const QString fFolders;
        static const QString tFolder;
        static const QString aName;

        static WBPersistenceManager* persistenceManager();
        static void destroy();

        virtual WBDocumentProxy* createDocument(const QString& pGroupName = ""
                , const QString& pName = ""
                , bool withEmptyPage = true
                , QString directory =QString()
                , int pageCount = 0
                , bool promptDialogIfExists = false);

        virtual WBDocumentProxy *createNewDocument(const QString& pGroupName = ""
                , const QString& pName = ""
                , bool withEmptyPage = true
                , QString directory =QString()
                , int pageCount = 0
                , bool promptDialogIfExists = false);

        virtual WBDocumentProxy* createDocumentFromDir(const QString& pDocumentDirectory
                                                       , const QString& pGroupName = ""
                , const QString& pName = ""
                , bool withEmptyPage = false
                , bool addTitlePage = false
                , bool promptDialogIfExists = false);

        virtual WBDocumentProxy* persistDocumentMetadata(WBDocumentProxy* pDocumentProxy);

        virtual WBDocumentProxy* duplicateDocument(WBDocumentProxy* pDocumentProxy);

        virtual void deleteDocument(WBDocumentProxy* pDocumentProxy);

        virtual void deleteDocumentScenes(WBDocumentProxy* pDocumentProxy, const QList<int>& indexes);

        virtual void duplicateDocumentScene(WBDocumentProxy* pDocumentProxy, int index);

        virtual void copyDocumentScene(WBDocumentProxy *from, int fromIndex, WBDocumentProxy *to, int toIndex);

        virtual void persistDocumentScene(WBDocumentProxy* pDocumentProxy,
                WBGraphicsScene* pScene, const int pSceneIndex);

        virtual WBGraphicsScene* createDocumentSceneAt(WBDocumentProxy* pDocumentProxy, int index, bool useUndoRedoStack = true);

        virtual void insertDocumentSceneAt(WBDocumentProxy* pDocumentProxy, WBGraphicsScene* scene, int index, bool persist = true);

        virtual void moveSceneToIndex(WBDocumentProxy* pDocumentProxy, int source, int target);

        virtual WBGraphicsScene* loadDocumentScene(WBDocumentProxy* pDocumentProxy, int sceneIndex);
        WBGraphicsScene *getDocumentScene(WBDocumentProxy* pDocumentProxy, int sceneIndex) {return mSceneCache.value(pDocumentProxy, sceneIndex);}
        void reassignDocProxy(WBDocumentProxy *newDocument, WBDocumentProxy *oldDocument);

//        QList<QPointer<WBDocumentProxy> > documentProxies;
        WBDocumentTreeNode *mDocumentTreeStructure;
        WBDocumentTreeModel *mDocumentTreeStructureModel;

        virtual QStringList allShapes();
        virtual QStringList allGips();
        virtual QStringList allImages(const QDir& dir);
        virtual QStringList allVideos(const QDir& dir);
        virtual QStringList allWidgets(const QDir& dir);

        QString generateUniqueDocumentPath();
        QString generateUniqueDocumentPath(const QString& baseFolder);

        bool addDirectoryContentToDocument(const QString& documentRootFolder, WBDocumentProxy* pDocument);

        void createDocumentProxiesStructure(bool interactive = false);
        void createDocumentProxiesStructure(const QFileInfoList &contentInfo, bool interactive = false);
        QDialog::DialogCode processInteractiveReplacementDialog(WBDocumentProxy *pProxy);

        QStringList documentSubDirectories()
        {
            return mDocumentSubDirectories;
        }

        virtual bool isEmpty(WBDocumentProxy* pDocumentProxy);
        virtual void purgeEmptyDocuments();

        bool addGraphicsWidgetToDocument(WBDocumentProxy *mDocumentProxy, QString path, QUuid objectUuid, QString& destinationPath);
        bool addFileToDocument(WBDocumentProxy* pDocumentProxy, QString path, const QString& subdir,  QUuid objectUuid, QString& destinationPath, QByteArray* data = NULL);

        bool mayHaveVideo(WBDocumentProxy* pDocumentProxy);
        bool mayHaveAudio(WBDocumentProxy* pDocumentProxy);
        bool mayHavePDF(WBDocumentProxy* pDocumentProxy);
        bool mayHaveSVGImages(WBDocumentProxy* pDocumentProxy);
        bool mayHaveWidget(WBDocumentProxy* pDocumentProxy);

        QString adjustDocumentVirtualPath(const QString &str);

        void closing();
        bool isSceneInCached(WBDocumentProxy *proxy, int index) const;

    signals:
        void proxyListChanged();

        void documentCreated(WBDocumentProxy* pDocumentProxy);
        void documentMetadataChanged(WBDocumentProxy* pDocumentProxy);
        void documentWillBeDeleted(WBDocumentProxy* pDocumentProxy);

        void documentSceneCreated(WBDocumentProxy* pDocumentProxy, int pIndex);
        void documentSceneWillBeDeleted(WBDocumentProxy* pDocumentProxy, int pIndex);

	private:
        int sceneCount(const WBDocumentProxy* pDocumentProxy);
        static QStringList getSceneFileNames(const QString& folder);
        void renamePage(WBDocumentProxy* pDocumentProxy,
                        const int sourceIndex, const int targetIndex);
        void copyPage(WBDocumentProxy* pDocumentProxy,
                      const int sourceIndex, const int targetIndex);
        void generatePathIfNeeded(WBDocumentProxy* pDocumentProxy);
        void checkIfDocumentRepositoryExists();

        void saveFoldersTreeToXml(QXmlStreamWriter &writer, const QModelIndex &parentIndex);
        void loadFolderTreeFromXml(const QString &path, const QDomElement &element);

        QString xmlFolderStructureFilename;

        WBSceneCache mSceneCache;
        QStringList mDocumentSubDirectories;
        QMutex mDeletedListMutex;
        bool mHasPurgedDocuments;
        QString mDocumentRepositoryPath;
        QString mFoldersXmlStorageName;

    private slots:
        void documentRepositoryChanged(const QString& path);

};

#endif /* WBPERSISTENCEMANAGER_H_ */
