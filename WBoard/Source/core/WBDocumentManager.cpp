#include "WBDocumentManager.h"

#include "frameworks/WBStringUtils.h"

#include "adaptors/WBExportFullPDF.h"
#include "adaptors/WBExportDocument.h"
#include "adaptors/WBExportWeb.h"
#include "adaptors/WBExportCFF.h"
#include "adaptors/WBExportDocumentSetAdaptor.h"
#include "adaptors/WBImportDocument.h"
#include "adaptors/WBImportPDF.h"
#include "adaptors/WBImportImage.h"
#include "adaptors/WBImportCFF.h"
#include "adaptors/WBImportDocumentSetAdaptor.h"

#include "domain/WBGraphicsScene.h"
#include "domain/WBGraphicsSvgItem.h"
#include "domain/WBGraphicsPixmapItem.h"

#include "document/WBDocumentProxy.h"
#include "document/WBDocumentController.h"
#include "board/WBBoardController.h"

#include "WBApplication.h"
#include "WBSettings.h"
#include "WBPersistenceManager.h"

#include "../adaptors/WBExportWeb.h"

#include "core/memcheck.h"

WBDocumentManager* WBDocumentManager::sDocumentManager = 0;

WBDocumentManager* WBDocumentManager::documentManager()
{
    if (!sDocumentManager)
    {
        sDocumentManager = new WBDocumentManager(qApp);
    }
    return sDocumentManager;
}


WBDocumentManager::WBDocumentManager(QObject *parent)
    :QObject(parent)
{
    QString dummyImages = tr("images");
    QString dummyVideos = tr("videos");
    QString dummyObjects = tr("objects");
    QString dummyWidgets = tr("widgets");

    //WBExportCFF* cffExporter = new WBExportCFF(this);
    WBExportFullPDF* exportFullPdf = new WBExportFullPDF(this);
    WBExportDocument* exportDocument = new WBExportDocument(this);

    WBExportDocumentSetAdaptor *exportDocumentSet = new WBExportDocumentSetAdaptor(this);
    mExportAdaptors.append(exportDocument);
    mExportAdaptors.append(exportDocumentSet);
    //mExportAdaptors.append(webPublished);
    mExportAdaptors.append(exportFullPdf);
    //mExportAdaptors.append(cffExporter);

//     WBExportWeb* exportWeb = new WBExportWeb(this);
//     mExportAdaptors.append(exportWeb);

    WBImportDocument* documentImport = new WBImportDocument(this);
    mImportAdaptors.append(documentImport);
    WBImportDocumentSetAdaptor *documentSetImport = new WBImportDocumentSetAdaptor(this);
    mImportAdaptors.append(documentSetImport);
    //WBImportPDF* pdfImport = new WBImportPDF(this);
    //mImportAdaptors.append(pdfImport);
    WBImportImage* imageImport = new WBImportImage(this);
    mImportAdaptors.append(imageImport);
    WBImportCFF* cffImport = new WBImportCFF(this);
    mImportAdaptors.append(cffImport);
}


WBDocumentManager::~WBDocumentManager()
{
    // NOOP
}


QStringList WBDocumentManager::importFileExtensions(bool notUbx)
{
    QStringList result;

    foreach (WBImportAdaptor *importAdaptor, mImportAdaptors)
    {
        //issue 1629 - NNE - 20131213 : add test to remove ubx extention if necessary
        if(!(notUbx && importAdaptor->supportedExtentions().at(0) == "ubx")){
            result << importAdaptor->supportedExtentions();
        }
    }
    return result;
}


QString WBDocumentManager::importFileFilter(bool notUbx)
{
    QString result;

    result += tr("All supported files (*.%1)").arg(importFileExtensions(notUbx).join(" *."));
    foreach (WBImportAdaptor *importAdaptor, mImportAdaptors)
    {
        if (importAdaptor->importFileFilter().length() > 0)
        {
            //issue 1629 - NNE - 20131213 : Add a test on ubx before put in the list
            if(!(notUbx && importAdaptor->supportedExtentions().at(0) == "ubx")){
                if (result.length())
                {
                    result += ";;";
                }

                result += importAdaptor->importFileFilter();
            }
        }
    }
    qDebug() << "import file filter" << result;
    return result;
}

QFileInfoList WBDocumentManager::importUbx(const QString &Incomingfile, const QString &destination)
{
    WBImportDocumentSetAdaptor *docSetAdaptor;
    foreach (WBImportAdaptor *curAdaptor, mImportAdaptors) {
        docSetAdaptor = qobject_cast<WBImportDocumentSetAdaptor*>(curAdaptor);
        if (docSetAdaptor) {
            break;
        }
    }
    if (!docSetAdaptor) {
        return QFileInfoList();
    }

    return docSetAdaptor->importData(Incomingfile, destination);
}

WBDocumentProxy* WBDocumentManager::importFile(const QFile& pFile, const QString& pGroup)
{
    QFileInfo fileInfo(pFile);

    foreach (WBImportAdaptor *adaptor, mImportAdaptors)
    {
        if (adaptor->supportedExtentions().lastIndexOf(fileInfo.suffix().toLower()) != -1)
        {
            WBDocumentProxy* document;
            WBApplication::setDisabled(true);

            if (adaptor->isDocumentBased())
            {
                WBDocumentBasedImportAdaptor* importAdaptor = (WBDocumentBasedImportAdaptor*)adaptor;

                document = importAdaptor->importFile(pFile, pGroup);

            }
            else
            {
                WBPageBasedImportAdaptor* importAdaptor = (WBPageBasedImportAdaptor*)adaptor;

                // Document import procedure.....
                QString documentName = QFileInfo(pFile.fileName()).completeBaseName();
                document = WBPersistenceManager::persistenceManager()->createDocument(pGroup
                                                                                      ,documentName
                                                                                      , false // Issue 1630 - CFA - 201410503 - suppression de la page vide ajoutee à l'import des pdfs
                                                                                      , QString()
                                                                                      , 0
                                                                                      , true);
				if (document)
				{
					QUuid uuid = QUuid::createUuid();
					QString filepath = pFile.fileName();
					if (importAdaptor->folderToCopy() != "")
					{
						bool b = WBPersistenceManager::persistenceManager()->addFileToDocument(document, pFile.fileName(), importAdaptor->folderToCopy(), uuid, filepath);
						if (!b)
						{
							WBApplication::setDisabled(false);
							return NULL;
						}
					}

					QList<WBGraphicsItem*> pages = importAdaptor->import(uuid, filepath);
					int nPage = 0;
					foreach(WBGraphicsItem* page, pages)
					{

						WBApplication::showMessage(tr("Inserting page %1 of %2").arg(++nPage).arg(pages.size()), true);
#ifdef Q_WS_MACX
						//Workaround for issue 912
						QApplication::processEvents();
#endif
						int pageIndex = document->pageCount();
						WBGraphicsScene* scene = WBPersistenceManager::persistenceManager()->createDocumentSceneAt(document, pageIndex);
						importAdaptor->placeImportedItemToScene(scene, page);
						WBPersistenceManager::persistenceManager()->persistDocumentScene(document, scene, pageIndex);
					}

					WBPersistenceManager::persistenceManager()->persistDocumentMetadata(document);
					WBApplication::showMessage(tr("Import successful."));
				}
                
            }

            WBApplication::setDisabled(false);
            return document;
        }

    }
    return NULL;
}


int WBDocumentManager::addFilesToDocument(WBDocumentProxy* document, QStringList fileNames)
{
    int nImportedDocuments = 0;
    foreach(const QString& fileName, fileNames)
    {
        WBApplication::showMessage(tr("Importing file %1").arg(fileName));

        QFile file(fileName);
        QFileInfo fileInfo(file);

        foreach (WBImportAdaptor *adaptor, mImportAdaptors)
        {
            if (adaptor->supportedExtentions().lastIndexOf(fileInfo.suffix().toLower()) != -1)
            {
                WBApplication::setDisabled(true);

                if (adaptor->isDocumentBased())
                {
                    //issue 1629 - NNE - 20131212 : Resolve a segfault, but for .ubx, actually
                    //the file will be not imported...
                    WBDocumentBasedImportAdaptor* importAdaptor = dynamic_cast<WBDocumentBasedImportAdaptor*>(adaptor);

                    if (importAdaptor && importAdaptor->addFileToDocument(document, file))
                        nImportedDocuments++;
                }
                else
                {
                    WBPageBasedImportAdaptor* importAdaptor = (WBPageBasedImportAdaptor*)adaptor;

                    QUuid uuid = QUuid::createUuid();
                    QString filepath = file.fileName();
                    if (importAdaptor->folderToCopy() != "")
                    {
                        bool b = WBPersistenceManager::persistenceManager()->addFileToDocument(document, file.fileName(), importAdaptor->folderToCopy() , uuid, filepath);
                        if (!b)
                        {
                            continue;
                        }
                    }

                    QList<WBGraphicsItem*> pages = importAdaptor->import(uuid, filepath);
                    int nPage = 0;
                    foreach(WBGraphicsItem* page, pages)
                    {
                        WBApplication::showMessage(tr("Inserting page %1 of %2").arg(++nPage).arg(pages.size()), true);
                        int pageIndex = document->pageCount();
                        WBGraphicsScene* scene = WBPersistenceManager::persistenceManager()->createDocumentSceneAt(document, pageIndex);
                        importAdaptor->placeImportedItemToScene(scene, page);
                        WBPersistenceManager::persistenceManager()->persistDocumentScene(document, scene, pageIndex);
                        WBApplication::boardController->insertThumbPage(pageIndex);
                    }

                    WBPersistenceManager::persistenceManager()->persistDocumentMetadata(document);
                    WBApplication::showMessage(tr("Import of file %1 successful.").arg(file.fileName()));
                    nImportedDocuments++;
                }

                WBApplication::setDisabled(false);
            }
        }
    }
    return nImportedDocuments;
}


int WBDocumentManager::addImageDirToDocument(const QDir& pDir, WBDocumentProxy* pDocument)
{
    QStringList filenames = pDir.entryList(QDir::Files | QDir::NoDotAndDotDot);

    filenames = WBStringUtils::sortByLastDigit(filenames);

    QStringList fileNames;

    foreach(QString f, filenames)
    {
        fileNames << pDir.absolutePath() + "/" + f;
    }

    return addFilesToDocument(pDocument, fileNames);

}


WBDocumentProxy* WBDocumentManager::importDir(const QDir& pDir, const QString& pGroup)
{
    WBDocumentProxy* doc = WBPersistenceManager::persistenceManager()->createDocument(pGroup, pDir.dirName());

    int result = addImageDirToDocument(pDir, doc);

    if (result > 0)
    {
        doc->setMetaData(WBSettings::documentGroupName, pGroup);
        doc->setMetaData(WBSettings::documentName, pDir.dirName());

        WBPersistenceManager::persistenceManager()->persistDocumentMetadata(doc);

        WBApplication::showMessage(tr("File %1 saved").arg(pDir.dirName()));

    }
    else
    {
        WBPersistenceManager::persistenceManager()->deleteDocument(doc);
    }

    return doc;
}


QList<WBExportAdaptor*> WBDocumentManager::supportedExportAdaptors()
{
    return mExportAdaptors;
}

void WBDocumentManager::emitDocumentUpdated(WBDocumentProxy* pDocument)
{
    emit documentUpdated(pDocument);
}
