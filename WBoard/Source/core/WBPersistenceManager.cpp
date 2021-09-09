#include "WBPersistenceManager.h"
#include "gui/WBMainWindow.h"

#include <QtXml>
#include <QVariant>
#include <QDomDocument>
#include <QXmlStreamWriter>
#include <QModelIndex>

#include "frameworks/WBPlatformUtils.h"
#include "frameworks/WBFileSystemUtils.h"

#include "core/WBApplication.h"
#include "core/WBSettings.h"
#include "core/WBSetting.h"
#include "core/WBForeignObjectsHandler.h"

#include "document/WBDocumentProxy.h"

#include "adaptors/WBExportPDF.h"
#include "adaptors/WBSvgSubsetAdaptor.h"
#include "adaptors/WBThumbnailAdaptor.h"
#include "adaptors/WBMetadataDcSubsetAdaptor.h"

#include "domain/WBGraphicsMediaItem.h"
#include "domain/WBGraphicsWidgetItem.h"
#include "domain/WBGraphicsPixmapItem.h"
#include "domain/WBGraphicsSvgItem.h"

#include "board/WBBoardController.h"
#include "board/WBBoardPaletteManager.h"

#include "document/WBDocumentController.h"

#include "core/memcheck.h"

const QString WBPersistenceManager::imageDirectory = "images"; // added to WBPersistenceManager::mAllDirectories
const QString WBPersistenceManager::objectDirectory = "objects"; // added to WBPersistenceManager::mAllDirectories
const QString WBPersistenceManager::widgetDirectory = "widgets"; // added to WBPersistenceManager::mAllDirectories
const QString WBPersistenceManager::videoDirectory = "videos"; // added to WBPersistenceManager::mAllDirectories
const QString WBPersistenceManager::audioDirectory = "audios"; // added to
const QString WBPersistenceManager::fileDirectory = "files"; // Issue 1683 (Evolution) - AOU - 20131206

const QString WBPersistenceManager::myDocumentsName = "MyDocuments";
const QString WBPersistenceManager::modelsName = "Models";
const QString WBPersistenceManager::untitledDocumentsName = "UntitledDocuments";
const QString WBPersistenceManager::fFolders = "folders.xml";
const QString WBPersistenceManager::tFolder = "folder";
const QString WBPersistenceManager::aName = "name";

WBPersistenceManager * WBPersistenceManager::sSingleton = 0;

WBPersistenceManager::WBPersistenceManager(QObject *pParent)
    : QObject(pParent)
    , mHasPurgedDocuments(false)
{

    xmlFolderStructureFilename = "model";

    mDocumentSubDirectories << imageDirectory;
    mDocumentSubDirectories << objectDirectory;
    mDocumentSubDirectories << widgetDirectory;
    mDocumentSubDirectories << videoDirectory;
    mDocumentSubDirectories << audioDirectory;
    mDocumentSubDirectories << fileDirectory; // Issue 1683 (Evolution) - AOU - 20131206

    mDocumentRepositoryPath = WBSettings::userDocumentDirectory();
    mFoldersXmlStorageName =  mDocumentRepositoryPath + "/" + fFolders;

    mDocumentTreeStructureModel = new WBDocumentTreeModel(this);
    createDocumentProxiesStructure();


    emit proxyListChanged();
}

WBPersistenceManager* WBPersistenceManager::persistenceManager()
{
    if (!sSingleton)
    {
        sSingleton = new WBPersistenceManager(WBApplication::staticMemoryCleaner);
    }

    return sSingleton;
}

void WBPersistenceManager::destroy()
{
    if (sSingleton)
        delete sSingleton;
    sSingleton = NULL;
}

WBPersistenceManager::~WBPersistenceManager()
{
}

void WBPersistenceManager::createDocumentProxiesStructure(bool interactive)
{
    mDocumentRepositoryPath = WBSettings::userDocumentDirectory();

    QDir rootDir(mDocumentRepositoryPath);
    rootDir.mkpath(rootDir.path());

    QFileInfoList contentList = rootDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time | QDir::Reversed);
    createDocumentProxiesStructure(contentList, interactive);

    if (QFileInfo(mFoldersXmlStorageName).exists()) {
        QDomDocument xmlDom;
        QFile inFile(mFoldersXmlStorageName);
        if (inFile.open(QIODevice::ReadOnly)) {
            QString domString(inFile.readAll());

            int errorLine = 0; int errorColumn = 0;
            QString errorStr;

            if (xmlDom.setContent(domString, &errorStr, &errorLine, &errorColumn)) {
                loadFolderTreeFromXml("", xmlDom.firstChildElement());
            } else {
                qDebug() << "Error reading content of " << mFoldersXmlStorageName << endl
                         << "Error:" << inFile.errorString()
                         << "Line:" << errorLine
                         << "Column:" << errorColumn;
            }
            inFile.close();
        } else {
            qDebug() << "Error reading" << mFoldersXmlStorageName << endl
                     << "Error:" << inFile.errorString();
        }
    }
}

void WBPersistenceManager::createDocumentProxiesStructure(const QFileInfoList &contentInfo, bool interactive)
{
    foreach(QFileInfo path, contentInfo)
    {
        QString fullPath = path.absoluteFilePath();

        QMap<QString, QVariant> metadatas = WBMetadataDcSubsetAdaptor::load(fullPath);

        QString docGroupName = metadatas.value(WBSettings::documentGroupName, QString()).toString();
        QString docName = metadatas.value(WBSettings::documentName, QString()).toString();

        if (docName.isEmpty()) {
            qDebug() << "Group name and document name are empty in WBPersistenceManager::createDocumentProxiesStructure()";
            continue;
        }

        QModelIndex parentIndex = mDocumentTreeStructureModel->goTo(docGroupName);
        if (!parentIndex.isValid()) {
            return;
        }

        WBDocumentProxy* docProxy = new WBDocumentProxy(fullPath, metadatas); // managed in WBDocumentTreeNode
        foreach(QString key, metadatas.keys()) {
            docProxy->setMetaData(key, metadatas.value(key));
        }

        if (metadatas.contains(WBSettings::documentPageCount))
        {
            docProxy->setPageCount(metadatas.value(WBSettings::documentPageCount).toInt());
        }
        else
        {
            int pageCount = sceneCount(docProxy);
            docProxy->setPageCount(pageCount);
        }

        if (!interactive)
            mDocumentTreeStructureModel->addDocument(docProxy, parentIndex);
        else
            processInteractiveReplacementDialog(docProxy);
    }
}

QDialog::DialogCode WBPersistenceManager::processInteractiveReplacementDialog(WBDocumentProxy *pProxy)
{
    //TODO claudio remove this hack necessary on double click on ubz file
    Qt::CursorShape saveShape;
    if(WBApplication::overrideCursor()){
        saveShape = WBApplication::overrideCursor()->shape();
        WBApplication::overrideCursor()->setShape(Qt::ArrowCursor);
    }
    else
        saveShape = Qt::ArrowCursor;

    QDialog::DialogCode result = QDialog::Rejected;

    if (WBApplication::documentController
            && WBApplication::documentController->mainWidget()) {
        QString docGroupName = pProxy->metaData(WBSettings::documentGroupName).toString();
        QModelIndex parentIndex = mDocumentTreeStructureModel->goTo(docGroupName);
        if (!parentIndex.isValid()) {
            WBApplication::overrideCursor()->setShape(saveShape);
            return QDialog::Rejected;
        }

        QStringList docList = mDocumentTreeStructureModel->nodeNameList(parentIndex);
        QString docName = pProxy->metaData(WBSettings::documentName).toString();

        if (docList.contains(docName)) {
            WBDocumentReplaceDialog *replaceDialog = new WBDocumentReplaceDialog(docName
                                                                                 , docList
                                                                                 , /*WBApplication::documentController->mainWidget()*/0
                                                                                 , Qt::Widget);
            if (replaceDialog->exec() == QDialog::Accepted)
            {
                result = QDialog::Accepted;
                QString resultName = replaceDialog->lineEditText();
                int i = docList.indexOf(resultName);
                if (i != -1) { //replace
                    QModelIndex replaceIndex = mDocumentTreeStructureModel->index(i, 0, parentIndex);
                    WBDocumentProxy *replaceProxy = mDocumentTreeStructureModel->proxyData(replaceIndex);

                    if (mDocumentTreeStructureModel->currentIndex() == replaceIndex)
                    {
                        WBApplication::documentController->selectDocument(pProxy, true, true);
                    }

                    if (replaceProxy) {
                        deleteDocument(replaceProxy);
                    }
                    if (replaceIndex.isValid()) {
                        mDocumentTreeStructureModel->removeRow(i, parentIndex);
                    }
                }
                pProxy->setMetaData(WBSettings::documentName, resultName);
                mDocumentTreeStructureModel->addDocument(pProxy, parentIndex);
            }
            replaceDialog->setParent(0);
            delete replaceDialog;
        } else {
            mDocumentTreeStructureModel->addDocument(pProxy, parentIndex);
            result = QDialog::Accepted;
        }

    }
    //TODO claudio the if is an hack
    if(WBApplication::overrideCursor())
        WBApplication::overrideCursor()->setShape(saveShape);

    return result;
}

QString WBPersistenceManager::adjustDocumentVirtualPath(const QString &str)
{
    QStringList pathList = str.split("/", QString::SkipEmptyParts);

    if (pathList.isEmpty()) {
        pathList.append(myDocumentsName);
        pathList.append(untitledDocumentsName);
    }

    if (pathList.first() != myDocumentsName
            && pathList.first() != WBSettings::trashedDocumentGroupNamePrefix
            && pathList.first() != modelsName) {
        pathList.prepend(myDocumentsName);
    }

    return pathList.join("/");
}

void WBPersistenceManager::closing()
{
    QDir rootDir(mDocumentRepositoryPath);
    rootDir.mkpath(rootDir.path());

    QFile outFile(mFoldersXmlStorageName);
    if (outFile.open(QIODevice::WriteOnly)) {
        QXmlStreamWriter writer(&outFile);
        writer.setAutoFormatting(true);
        writer.writeStartDocument();
        writer.writeStartElement("content");
        saveFoldersTreeToXml(writer, QModelIndex());
        writer.writeEndElement();
        writer.writeEndDocument();

        outFile.close();
    } else {
        qDebug() << "failed to open document" <<  mFoldersXmlStorageName << "for writing" << endl
                 << "Error string:" << outFile.errorString();
    }
}

bool WBPersistenceManager::isSceneInCached(WBDocumentProxy *proxy, int index) const
{
    return mSceneCache.contains(proxy, index);
}

QStringList WBPersistenceManager::allShapes()
{
    QString shapeLibraryPath = WBSettings::settings()->applicationShapeLibraryDirectory();

    QDir dir(shapeLibraryPath);

    if (!dir.exists())
        dir.mkpath(shapeLibraryPath);

    QStringList files = dir.entryList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
    QStringList paths;

    foreach(QString file, files)
    {
        paths.append(shapeLibraryPath + QString("/") + file);
    }

    return paths;
}

QStringList WBPersistenceManager::allGips()
{
    QString gipLibraryPath = WBSettings::settings()->userGipLibraryDirectory();

    QDir dir(gipLibraryPath);

    QStringList files = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    QStringList paths;

    foreach(QString file, files)
    {
        QFileInfo fi(file);

        if (WBSettings::settings()->widgetFileExtensions.contains(fi.suffix()))
            paths.append(dir.path() + QString("/") + file);
    }

    return paths;
}

QStringList WBPersistenceManager::allImages(const QDir& dir)
{
    if (!dir.exists())
        dir.mkpath(dir.path());

    QStringList files = dir.entryList(QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot, QDir::Name);
    QStringList paths;

    foreach(QString file, files)
    {
        paths.append(dir.path() + QString("/") + file);
    }

    return paths;
}


QStringList WBPersistenceManager::allVideos(const QDir& dir)
{
    if (!dir.exists())
        dir.mkpath(dir.path());

    QStringList files = dir.entryList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
    QStringList paths;

    foreach(QString file, files)
    {
        paths.append(dir.path() + QString("/") + file);
    }

    return paths;
}


QStringList WBPersistenceManager::allWidgets(const QDir& dir)
{
    if (!dir.exists())
        dir.mkpath(dir.path());

    QStringList files = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    QStringList paths;

    foreach(QString file, files)
    {
        QFileInfo fi(file);

        if (WBSettings::settings()->widgetFileExtensions.contains(fi.suffix()))
            paths.append(dir.path() + QString("/") + file);
    }

    return paths;
}


WBDocumentProxy* WBPersistenceManager::createDocument(const QString& pGroupName
                                                      , const QString& pName
                                                      , bool withEmptyPage
                                                      , QString directory
                                                      , int pageCount
                                                      , bool promptDialogIfExists)
{
    WBDocumentProxy *doc;
    if(directory.length() != 0 ){
        doc = new WBDocumentProxy(directory); // deleted in WBPersistenceManager::destructor
        doc->setPageCount(pageCount);
    }
    else{
        checkIfDocumentRepositoryExists();
        doc = new WBDocumentProxy();
    }

    if (pGroupName.length() > 0)
    {
        doc->setMetaData(WBSettings::documentGroupName, pGroupName);
    }

    if (pName.length() > 0)
    {
        doc->setMetaData(WBSettings::documentName, pName);
    }

    doc->setMetaData(WBSettings::documentVersion, WBSettings::currentFileVersion);
    QString currentDate =  WBStringUtils::toUtcIsoDateTime(QDateTime::currentDateTime());
    doc->setMetaData(WBSettings::documentUpdatedAt,currentDate);
    doc->setMetaData(WBSettings::documentDate,currentDate);

    if (withEmptyPage) {
        createDocumentSceneAt(doc, 0);
    }
    else{
        this->generatePathIfNeeded(doc);
        QDir dir(doc->persistencePath());
        if (!dir.mkpath(doc->persistencePath()))
        {
            return 0; // if we can't create the path, abort function.
        }
    }

    bool addDoc = false;
    if (!promptDialogIfExists) {
        addDoc = true;
        mDocumentTreeStructureModel->addDocument(doc);
    } else if (processInteractiveReplacementDialog(doc) == QDialog::Accepted) {
        addDoc = true;
    }
    if (addDoc) {
        emit proxyListChanged();
    } else {
        deleteDocument(doc);
        doc = 0;
    }

    return doc;
}

WBDocumentProxy* WBPersistenceManager::createNewDocument(const QString& pGroupName
                                                      , const QString& pName
                                                      , bool withEmptyPage
                                                      , QString directory
                                                      , int pageCount
                                                      , bool promptDialogIfExists)
{
    WBDocumentProxy *resultDoc = createDocument(pGroupName, pName, withEmptyPage, directory, pageCount, promptDialogIfExists);
    if (resultDoc) {
        mDocumentTreeStructureModel->markDocumentAsNew(resultDoc);
    }

    return resultDoc;
}

WBDocumentProxy* WBPersistenceManager::createDocumentFromDir(const QString& pDocumentDirectory
                                                             , const QString& pGroupName
                                                             , const QString& pName
                                                             , bool withEmptyPage
                                                             , bool addTitlePage
                                                             , bool promptDialogIfExists)
{
    checkIfDocumentRepositoryExists();

    WBDocumentProxy* doc = new WBDocumentProxy(pDocumentDirectory); // deleted in WBPersistenceManager::destructor

    if (pGroupName.length() > 0)
    {
        doc->setMetaData(WBSettings::documentGroupName, pGroupName);
    }

    if (pName.length() > 0)
    {
        doc->setMetaData(WBSettings::documentName, pName);
    }

    QMap<QString, QVariant> metadatas = WBMetadataDcSubsetAdaptor::load(pDocumentDirectory);

    if(withEmptyPage) createDocumentSceneAt(doc, 0);
    if(addTitlePage) persistDocumentScene(doc, mSceneCache.createScene(doc, 0, false), 0);

    foreach(QString key, metadatas.keys())
    {
        doc->setMetaData(key, metadatas.value(key));
    }

    doc->setUuid(QUuid::createUuid());
    doc->setPageCount(sceneCount(doc));

    for(int i = 0; i < doc->pageCount(); i++)
    {
        WBSvgSubsetAdaptor::setSceneUuid(doc, i, QUuid::createUuid());
    }

    //work around the
    bool addDoc = false;
    if (!promptDialogIfExists) {
        addDoc = true;
        mDocumentTreeStructureModel->addDocument(doc);
    } else if (processInteractiveReplacementDialog(doc) == QDialog::Accepted) {
        addDoc = true;
    }
    if (addDoc) {
        WBMetadataDcSubsetAdaptor::persist(doc);
        emit proxyListChanged();
        emit documentCreated(doc);
    } else {
        deleteDocument(doc);
        doc = 0;
    }

    return doc;
}


void WBPersistenceManager::deleteDocument(WBDocumentProxy* pDocumentProxy)
{
    checkIfDocumentRepositoryExists();

    emit documentWillBeDeleted(pDocumentProxy);

    if (QFileInfo(pDocumentProxy->persistencePath()).exists())
        WBFileSystemUtils::deleteDir(pDocumentProxy->persistencePath());

    mSceneCache.removeAllScenes(pDocumentProxy);

    pDocumentProxy->deleteLater();
}

WBDocumentProxy* WBPersistenceManager::duplicateDocument(WBDocumentProxy* pDocumentProxy)
{
    checkIfDocumentRepositoryExists();

    WBDocumentProxy *copy = new WBDocumentProxy(); // deleted in WBPersistenceManager::destructor

    generatePathIfNeeded(copy);

    WBFileSystemUtils::copyDir(pDocumentProxy->persistencePath(), copy->persistencePath());

    // regenerate scenes UUIDs
    for(int i = 0; i < pDocumentProxy->pageCount(); i++)
    {
        WBSvgSubsetAdaptor::setSceneUuid(pDocumentProxy, i, QUuid::createUuid());
    }

    foreach(QString key, pDocumentProxy->metaDatas().keys())
    {
        copy->setMetaData(key, pDocumentProxy->metaDatas().value(key));
    }    

    copy->setMetaData(WBSettings::documentName,
            pDocumentProxy->metaData(WBSettings::documentName).toString() + " " + tr("(copy)"));

    copy->setUuid(QUuid::createUuid());

    persistDocumentMetadata(copy);

    copy->setPageCount(sceneCount(copy));

    emit proxyListChanged();

    emit documentCreated(copy);

    return copy;

}


void WBPersistenceManager::deleteDocumentScenes(WBDocumentProxy* proxy, const QList<int>& indexes)
{
    checkIfDocumentRepositoryExists();

    int pageCount = WBPersistenceManager::persistenceManager()->sceneCount(proxy);

    QList<int> compactedIndexes;

    foreach(int index, indexes)
    {
        if (!compactedIndexes.contains(index))
            compactedIndexes.append(index);
    }

    if (compactedIndexes.size() == pageCount)
    {
        deleteDocument(proxy);
        return;
    }

    if (compactedIndexes.size() == 0)
        return;

    foreach(int index, compactedIndexes)
    {
        emit documentSceneWillBeDeleted(proxy, index);
    }

    QString sourceName = proxy->metaData(WBSettings::documentName).toString();
    WBDocumentProxy *trashDocProxy = createDocument(WBSettings::trashedDocumentGroupNamePrefix/* + sourceGroupName*/, sourceName, false);

    foreach(int index, compactedIndexes)
    {
        WBGraphicsScene *scene = loadDocumentScene(proxy, index);
        if (scene)
        {
            //scene is about to move into new document
            foreach (QUrl relativeFile, scene->relativeDependencies())
            {
                QString source = scene->document()->persistencePath() + "/" + relativeFile.toString();
                QString target = trashDocProxy->persistencePath() + "/" + relativeFile.toString();

                QFileInfo fi(target);
                QDir d = fi.dir();

                d.mkpath(d.absolutePath());
                QFile::copy(source, target);
            }

            insertDocumentSceneAt(trashDocProxy, scene, trashDocProxy->pageCount());
        }
    }

    for (int i = 1; i < pageCount; i++)
    {
        renamePage(trashDocProxy, i , i - 1);
    }

    foreach(int index, compactedIndexes)
    {
        QString svgFileName = proxy->persistencePath() + WBFileSystemUtils::digitFileFormat("/page%1.svg", index);

        QFile::remove(svgFileName);

        QString thumbFileName = proxy->persistencePath() + WBFileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", index);

        QFile::remove(thumbFileName);

        mSceneCache.removeScene(proxy, index);

        proxy->decPageCount();

    }

    qSort(compactedIndexes);

    int offset = 1;

    for (int i = compactedIndexes.at(0) + 1; i < pageCount; i++)
    {
        if(compactedIndexes.contains(i))
        {
            offset++;
        }
        else
        {
            renamePage(proxy, i , i - offset);

            mSceneCache.moveScene(proxy, i, i - offset);

        }
    }
}


void WBPersistenceManager::duplicateDocumentScene(WBDocumentProxy* proxy, int index)
{
    checkIfDocumentRepositoryExists();

    int pageCount = WBPersistenceManager::persistenceManager()->sceneCount(proxy);

    for (int i = pageCount; i > index + 1; i--)
    {
        renamePage(proxy, i - 1 , i);

        mSceneCache.moveScene(proxy, i - 1, i);

    }

    copyPage(proxy, index , index + 1);

    //TODO: write a proper way to handle object on disk
    WBGraphicsScene *scene = loadDocumentScene(proxy, index + 1);

    foreach(QGraphicsItem* item, scene->items())
    {
        WBGraphicsMediaItem *mediaItem = qgraphicsitem_cast<WBGraphicsMediaItem*> (item);

        if (mediaItem){
            QString source = mediaItem->mediaFileUrl().toLocalFile();
            QString destination = source;
            QUuid newUuid = QUuid::createUuid();
            QString fileName = QFileInfo(source).completeBaseName();
            destination = destination.replace(fileName,newUuid.toString());
            QFile::copy(source,destination);
            mediaItem->setMediaFileUrl(QUrl::fromLocalFile(destination));
            continue;
        }

        WBGraphicsWidgetItem* widget = qgraphicsitem_cast<WBGraphicsWidgetItem*>(item);
        if(widget){
            QUuid newUUid = QUuid::createUuid();
            QString newUUidString = newUUid.toString().remove("{").remove("}");
            QString actualUuidString = widget->uuid().toString().remove("{").remove("}");

            QString widgetSourcePath = proxy->persistencePath() + "/" + WBPersistenceManager::widgetDirectory + "/{" + actualUuidString + "}.wgt";
            QString screenshotSourcePath = proxy->persistencePath() + "/" +  WBPersistenceManager::widgetDirectory + "/" + actualUuidString + ".png";

            QString widgetDestinationPath = widgetSourcePath;
            widgetDestinationPath = widgetDestinationPath.replace(actualUuidString,newUUidString);
            QString screenshotDestinationPath = screenshotSourcePath;
            screenshotDestinationPath = screenshotDestinationPath.replace(actualUuidString,newUUidString);

            WBFileSystemUtils::copyDir(widgetSourcePath,widgetDestinationPath);
            QFile::copy(screenshotSourcePath,screenshotDestinationPath);

            widget->setUuid(newUUid);

            widget->widgetUrl(QUrl::fromLocalFile(widgetDestinationPath));

            continue;
        }

        WBGraphicsPixmapItem* pixmapItem = qgraphicsitem_cast<WBGraphicsPixmapItem*>(item);
        if(pixmapItem){
            QString source = proxy->persistencePath() + "/" +  WBPersistenceManager::imageDirectory + "/" + pixmapItem->uuid().toString() + ".png";
            QString destination = source;
            QUuid newUuid = QUuid::createUuid();
            QString fileName = QFileInfo(source).completeBaseName();
            destination = destination.replace(fileName,newUuid.toString());
            QFile::copy(source,destination);
            pixmapItem->setUuid(newUuid);
            continue;
        }

        WBGraphicsSvgItem* svgItem = qgraphicsitem_cast<WBGraphicsSvgItem*>(item);
        if(svgItem){
            QString source = proxy->persistencePath() + "/" +  WBPersistenceManager::imageDirectory + "/" + svgItem->uuid().toString() + ".svg";
            QString destination = source;
            QUuid newUuid = QUuid::createUuid();
            QString fileName = QFileInfo(source).completeBaseName();
            destination = destination.replace(fileName,newUuid.toString());
            QFile::copy(source,destination);
            svgItem->setUuid(newUuid);
            continue;
        }

    }
    scene->setModified(true);

    persistDocumentScene(proxy,scene, index + 1);

    proxy->incPageCount();

    emit documentSceneCreated(proxy, index + 1);
}

void WBPersistenceManager::copyDocumentScene(WBDocumentProxy *from, int fromIndex, WBDocumentProxy *to, int toIndex)
{
    if (from == to && toIndex <= fromIndex) {
        qDebug() << "operation is not supported" << Q_FUNC_INFO;
        return;
    }

    checkIfDocumentRepositoryExists();

    for (int i = to->pageCount(); i > toIndex; i--) {
        renamePage(to, i - 1, i);
        mSceneCache.moveScene(to, i - 1, i);
    }

    WBForeighnObjectsHandler hl;
    hl.copyPage(QUrl::fromLocalFile(from->persistencePath()), fromIndex,
                QUrl::fromLocalFile(to->persistencePath()), toIndex);

    to->incPageCount();

    QString thumbTmp(from->persistencePath() + WBFileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", fromIndex));
    QString thumbTo(to->persistencePath() + WBFileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", toIndex));

    QFile::remove(thumbTo);
    QFile::copy(thumbTmp, thumbTo);

    Q_ASSERT(QFileInfo(thumbTmp).exists());
    Q_ASSERT(QFileInfo(thumbTo).exists());
    const QPixmap *pix = new QPixmap(thumbTmp);
    WBDocumentController *ctrl = WBApplication::documentController;
    ctrl->addPixmapAt(pix, toIndex);
    ctrl->TreeViewSelectionChanged(ctrl->firstSelectedTreeIndex(), QModelIndex());

//    emit documentSceneCreated(to, toIndex + 1);
}


WBGraphicsScene* WBPersistenceManager::createDocumentSceneAt(WBDocumentProxy* proxy, int index, bool useUndoRedoStack)
{
    int count = sceneCount(proxy);

    for(int i = count - 1; i >= index; i--)
        renamePage(proxy, i , i + 1);

    mSceneCache.shiftUpScenes(proxy, index, count -1);

    WBGraphicsScene *newScene = mSceneCache.createScene(proxy, index, useUndoRedoStack);

    newScene->setBackground(WBSettings::settings()->isDarkBackground(),
            WBSettings::settings()->WBSettings::pageBackground());

    newScene->setBackgroundGridSize(WBSettings::settings()->crossSize);

    persistDocumentScene(proxy, newScene, index);

    proxy->incPageCount();

    emit documentSceneCreated(proxy, index);

    return newScene;
}


void WBPersistenceManager::insertDocumentSceneAt(WBDocumentProxy* proxy, WBGraphicsScene* scene, int index, bool persist)
{
    scene->setDocument(proxy);

    int count = sceneCount(proxy);

    for(int i = count - 1; i >= index; i--)
    {
        renamePage(proxy, i , i + 1);
    }

    mSceneCache.shiftUpScenes(proxy, index, count -1);

    mSceneCache.insert(proxy, index, scene);

    if (persist) {
        persistDocumentScene(proxy, scene, index);
    }

    proxy->incPageCount();

    emit documentSceneCreated(proxy, index);

}


void WBPersistenceManager::moveSceneToIndex(WBDocumentProxy* proxy, int source, int target)
{
    checkIfDocumentRepositoryExists();

    if (source == target)
        return;

    QFile svgTmp(proxy->persistencePath() + WBFileSystemUtils::digitFileFormat("/page%1.svg", source));
    svgTmp.rename(proxy->persistencePath() + WBFileSystemUtils::digitFileFormat("/page%1.tmp", target));

    QFile thumbTmp(proxy->persistencePath() + WBFileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", source));
    thumbTmp.rename(proxy->persistencePath() + WBFileSystemUtils::digitFileFormat("/page%1.thumbnail.tmp", target));

    if (source < target)
    {
        for (int i = source + 1; i <= target; i++)
        {
            renamePage(proxy, i , i - 1);
        }
    }
    else
    {
        for (int i = source - 1; i >= target; i--)
        {
            renamePage(proxy, i , i + 1);
        }
    }

    QFile svg(proxy->persistencePath() + WBFileSystemUtils::digitFileFormat("/page%1.tmp", target));
    svg.rename(proxy->persistencePath() + WBFileSystemUtils::digitFileFormat("/page%1.svg", target));

    QFile thumb(proxy->persistencePath() + WBFileSystemUtils::digitFileFormat("/page%1.thumbnail.tmp", target));
    thumb.rename(proxy->persistencePath() + WBFileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", target));

    mSceneCache.moveScene(proxy, source, target);
}


WBGraphicsScene* WBPersistenceManager::loadDocumentScene(WBDocumentProxy* proxy, int sceneIndex)
{
    if (mSceneCache.contains(proxy, sceneIndex))
        return mSceneCache.value(proxy, sceneIndex);
    else {
        WBGraphicsScene* scene = WBSvgSubsetAdaptor::loadScene(proxy, sceneIndex);
        if(!scene){
            createDocumentSceneAt(proxy,0);
            scene = WBSvgSubsetAdaptor::loadScene(proxy, 0);
        }

        if (scene)
            mSceneCache.insert(proxy, sceneIndex, scene);

        return scene;
    }
}

void WBPersistenceManager::reassignDocProxy(WBDocumentProxy *newDocument, WBDocumentProxy *oldDocument)
{
    return mSceneCache.reassignDocProxy(newDocument, oldDocument);
}

void WBPersistenceManager::persistDocumentScene(WBDocumentProxy* pDocumentProxy, WBGraphicsScene* pScene, const int pSceneIndex)
{
    checkIfDocumentRepositoryExists();

    pScene->deselectAllItems();

    generatePathIfNeeded(pDocumentProxy);

    QDir dir(pDocumentProxy->persistencePath());
    dir.mkpath(pDocumentProxy->persistencePath());

    if (pDocumentProxy->isModified())
        WBMetadataDcSubsetAdaptor::persist(pDocumentProxy);

    if (pScene->isModified())
    {
        WBSvgSubsetAdaptor::persistScene(pDocumentProxy, pScene, pSceneIndex);

        WBThumbnailAdaptor::persistScene(pDocumentProxy, pScene, pSceneIndex);

        pScene->setModified(false);
    }

    mSceneCache.insert(pDocumentProxy, pSceneIndex, pScene);
}


WBDocumentProxy* WBPersistenceManager::persistDocumentMetadata(WBDocumentProxy* pDocumentProxy)
{
    WBMetadataDcSubsetAdaptor::persist(pDocumentProxy);

    emit documentMetadataChanged(pDocumentProxy);

    return pDocumentProxy;
}


void WBPersistenceManager::renamePage(WBDocumentProxy* pDocumentProxy, const int sourceIndex, const int targetIndex)
{
    QFile svg(pDocumentProxy->persistencePath() + WBFileSystemUtils::digitFileFormat("/page%1.svg", sourceIndex));
    svg.rename(pDocumentProxy->persistencePath() + WBFileSystemUtils::digitFileFormat("/page%1.svg",  targetIndex));

    QFile thumb(pDocumentProxy->persistencePath() + WBFileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", sourceIndex));
    thumb.rename(pDocumentProxy->persistencePath() + WBFileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", targetIndex));
}


void WBPersistenceManager::copyPage(WBDocumentProxy* pDocumentProxy, const int sourceIndex, const int targetIndex)
{
    QFile svg(pDocumentProxy->persistencePath() + WBFileSystemUtils::digitFileFormat("/page%1.svg",sourceIndex));
    svg.copy(pDocumentProxy->persistencePath() + WBFileSystemUtils::digitFileFormat("/page%1.svg", targetIndex));

    WBSvgSubsetAdaptor::setSceneUuid(pDocumentProxy, targetIndex, QUuid::createUuid());

    QFile thumb(pDocumentProxy->persistencePath() + WBFileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", sourceIndex));
    thumb.copy(pDocumentProxy->persistencePath() + WBFileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", targetIndex));
}


int WBPersistenceManager::sceneCount(const WBDocumentProxy* proxy)
{
    const QString pPath = proxy->persistencePath();

    int pageIndex = 0;
    bool moreToProcess = true;
    bool addedMissingZeroPage = false;

    while (moreToProcess)
    {
        QString fileName = pPath + WBFileSystemUtils::digitFileFormat("/page%1.svg", pageIndex);

        QFile file(fileName);

        if (file.exists())
        {
            pageIndex++;
        }
        else
        {
            moreToProcess = false;
        }
    }

    if(pageIndex == 1 && addedMissingZeroPage){
        // increment is done only to check if there are other pages than the missing zero page
        // This situation means -> no pages on the document
        return 0;
    }

    return pageIndex;
}

QStringList WBPersistenceManager::getSceneFileNames(const QString& folder)
{
    QDir dir(folder, "page???.svg", QDir::Name, QDir::Files);
    return dir.entryList();
}

QString WBPersistenceManager::generateUniqueDocumentPath(const QString& baseFolder)
{
    QDateTime now = QDateTime::currentDateTime();
    QString dirName = now.toString("yyyy-MM-dd hh-mm-ss.zzz");

    return baseFolder + QString("/WBoard Document %1").arg(dirName);
}

QString WBPersistenceManager::generateUniqueDocumentPath()
{
    return generateUniqueDocumentPath(WBSettings::userDocumentDirectory());
}


void WBPersistenceManager::generatePathIfNeeded(WBDocumentProxy* pDocumentProxy)
{
    if (pDocumentProxy->persistencePath().length() == 0)
    {
        pDocumentProxy->setPersistencePath(generateUniqueDocumentPath());
    }
}


bool WBPersistenceManager::addDirectoryContentToDocument(const QString& documentRootFolder, WBDocumentProxy* pDocument)
{
    QStringList sourceScenes = getSceneFileNames(documentRootFolder);
    if (sourceScenes.empty())
        return false;

    int targetPageCount = pDocument->pageCount();

    for(int sourceIndex = 0 ; sourceIndex < sourceScenes.size(); sourceIndex++)
    {
        int targetIndex = targetPageCount + sourceIndex;

        QFile svg(documentRootFolder + "/" + sourceScenes[sourceIndex]);
        if (!svg.copy(pDocument->persistencePath() + WBFileSystemUtils::digitFileFormat("/page%1.svg", targetIndex)))
            return false;

        WBSvgSubsetAdaptor::setSceneUuid(pDocument, targetIndex, QUuid::createUuid());

        QFile thumb(documentRootFolder + WBFileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", sourceIndex));
        // We can ignore error in this case, thumbnail will be genarated
        thumb.copy(pDocument->persistencePath() + WBFileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", targetIndex));
    }

    foreach(QString dir, mDocumentSubDirectories)
    {
        qDebug() << "copying " << documentRootFolder << "/" << dir << " to " << pDocument->persistencePath() << "/" + dir;

        QDir srcDir(documentRootFolder + "/" + dir);
        if (srcDir.exists())
            if (!WBFileSystemUtils::copyDir(documentRootFolder + "/" + dir, pDocument->persistencePath() + "/" + dir))
                return false;
    }

    pDocument->setPageCount(sceneCount(pDocument));

    //issue NC - NNE - 20131213 : At this point, all is well done.
    return true;
}


bool WBPersistenceManager::isEmpty(WBDocumentProxy* pDocumentProxy)
{
    if(!pDocumentProxy)
        return true;

    if (pDocumentProxy->pageCount() > 1)
        return false;

    WBGraphicsScene *theSoleScene = WBSvgSubsetAdaptor::loadScene(pDocumentProxy, 0);

    bool empty = false;

    if (theSoleScene)
    {
        empty = theSoleScene->isEmpty();
        delete theSoleScene;
    }
    else
    {
        empty = true;
    }

    return empty;
}


void WBPersistenceManager::purgeEmptyDocuments()
{
    QList<WBDocumentProxy*> toBeDeleted;

    foreach(WBDocumentProxy* docProxy, mDocumentTreeStructureModel->newDocuments())
    {
        if (isEmpty(docProxy))
        {
            toBeDeleted << docProxy;
        }
    }

    foreach(WBDocumentProxy* docProxy, toBeDeleted)
    {
        deleteDocument(docProxy);
    }
}

bool WBPersistenceManager::addFileToDocument(WBDocumentProxy* pDocumentProxy,
                                                     QString path,
                                                     const QString& subdir,
                                                     QUuid objectUuid,
                                                     QString& destinationPath,
                                                     QByteArray* data)
{
    Q_ASSERT(path.length());
    QFileInfo fi(path);

    if (!pDocumentProxy || objectUuid.isNull())
        return false;
    if (data == NULL && !fi.exists())
        return false;

    qDebug() << fi.suffix();

    QString fileName = subdir + "/" + objectUuid.toString() + "." + fi.suffix();

    destinationPath = pDocumentProxy->persistencePath() + "/" + fileName;

    if (!QFile::exists(destinationPath))
    {
        QDir dir;
        dir.mkdir(pDocumentProxy->persistencePath() + "/" + subdir);
        if (!QFile::exists(pDocumentProxy->persistencePath() + "/" + subdir))
            return false;

        if (data == NULL)
        {
            QFile source(path);
            return source.copy(destinationPath);
        }
        else
        {
            QFile newFile(destinationPath);

            if (newFile.open(QIODevice::WriteOnly))
            {
                qint64 n = newFile.write(*data);
                newFile.flush();
                newFile.close();
                return n == data->size();
            }
            else
            {
                return false;
            }
        }
    }
    else
    {
        return false;
    }
}

bool WBPersistenceManager::addGraphicsWidgetToDocument(WBDocumentProxy *pDocumentProxy,
                                                       QString path,
                                                       QUuid objectUuid,
                                                       QString& destinationPath)
{
    QFileInfo fi(path);

    if (!fi.exists() || !pDocumentProxy || objectUuid.isNull())
        return false;

    QString widgetRootDir = path;
    QString extension = QFileInfo(widgetRootDir).suffix();

    destinationPath = pDocumentProxy->persistencePath() + "/" + widgetDirectory +  "/" + objectUuid.toString() + "." + extension;

    if (!QFile::exists(destinationPath)) {
        QDir dir;
        if (!dir.mkpath(destinationPath))
            return false;
        return WBFileSystemUtils::copyDir(widgetRootDir, destinationPath);
    }
    else
        return false;
}


void WBPersistenceManager::documentRepositoryChanged(const QString& path)
{
    Q_UNUSED(path);
    checkIfDocumentRepositoryExists();
}


void WBPersistenceManager::checkIfDocumentRepositoryExists()
{
    QDir rp(mDocumentRepositoryPath);

    if (!rp.exists())
    {
        // we have lost the document repository ..

        QString humanPath = QDir::cleanPath(mDocumentRepositoryPath);
        humanPath = QDir::toNativeSeparators(humanPath);

        WBApplication::mainWindow->warning(tr("Document Repository Loss"),tr("WBoard has lost access to the document repository '%1'. Unfortunately the application must shut down to avoid data corruption. Latest changes may be lost as well.").arg(humanPath));

        WBApplication::quit();
    }
}

void WBPersistenceManager::saveFoldersTreeToXml(QXmlStreamWriter &writer, const QModelIndex &parentIndex)
{
    for (int i = 0; i < mDocumentTreeStructureModel->rowCount(parentIndex); i++)
    {
        QModelIndex currentIndex = mDocumentTreeStructureModel->index(i, 0, parentIndex);
        if (mDocumentTreeStructureModel->isCatalog(currentIndex))
        {
            writer.writeStartElement(tFolder);
            writer.writeAttribute(aName, mDocumentTreeStructureModel->nodeFromIndex(currentIndex)->nodeName());
            saveFoldersTreeToXml(writer, currentIndex);
            writer.writeEndElement();
        }
    }
}

void WBPersistenceManager::loadFolderTreeFromXml(const QString &path, const QDomElement &element)
{

    QDomElement iterElement = element.firstChildElement();
    while(!iterElement.isNull())
    {
        QString leafPath;
        if (tFolder == iterElement.tagName())
        {
            leafPath = iterElement.attribute(aName);

            if (!leafPath.isEmpty())
            {
                mDocumentTreeStructureModel->goTo(path + "/" + leafPath);
                if (!iterElement.firstChildElement().isNull())
                    loadFolderTreeFromXml(path + "/" +  leafPath, iterElement);
            }
        }
        iterElement = iterElement.nextSiblingElement();
    }
}

bool WBPersistenceManager::mayHaveVideo(WBDocumentProxy* pDocumentProxy)
{
    QDir videoDir(pDocumentProxy->persistencePath() + "/" + WBPersistenceManager::videoDirectory);

    return videoDir.exists() && videoDir.entryInfoList().length() > 0;
}

bool WBPersistenceManager::mayHaveAudio(WBDocumentProxy* pDocumentProxy)
{
    QDir audioDir(pDocumentProxy->persistencePath() + "/" + WBPersistenceManager::audioDirectory);

    return audioDir.exists() && audioDir.entryInfoList().length() > 0;
}

bool WBPersistenceManager::mayHavePDF(WBDocumentProxy* pDocumentProxy)
{
    QDir objectDir(pDocumentProxy->persistencePath() + "/" + WBPersistenceManager::objectDirectory);

    QStringList filters;
    filters << "*.pdf";

    return objectDir.exists() && objectDir.entryInfoList(filters).length() > 0;
}


bool WBPersistenceManager::mayHaveSVGImages(WBDocumentProxy* pDocumentProxy)
{
    QDir imageDir(pDocumentProxy->persistencePath() + "/" + WBPersistenceManager::imageDirectory);

    QStringList filters;
    filters << "*.svg";

    return imageDir.exists() && imageDir.entryInfoList(filters).length() > 0;
}


bool WBPersistenceManager::mayHaveWidget(WBDocumentProxy* pDocumentProxy)
{
    QDir widgetDir(pDocumentProxy->persistencePath() + "/" + WBPersistenceManager::widgetDirectory);

    return widgetDir.exists() && widgetDir.entryInfoList(QDir::Dirs).length() > 0;
}
