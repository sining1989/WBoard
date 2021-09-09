#include <QDir>
#include <QList>

#include "core/WBApplication.h"
#include "core/WBPersistenceManager.h"
#include "core/WBDocumentManager.h"
#include "core/WBPersistenceManager.h"
#include "document/WBDocumentProxy.h"
#include "domain/WBGraphicsPDFItem.h"
#include "frameworks/WBFileSystemUtils.h"
#include "pdf/PDFRenderer.h"

#include "WBCFFSubsetAdaptor.h"
#include "WBImportCFF.h"

#include "globals/WBGlobals.h"

THIRD_PARTY_WARNINGS_DISABLE
#include "quazip.h"
#include "quazipfile.h"
#include "quazipfileinfo.h"
THIRD_PARTY_WARNINGS_ENABLE

#include "core/memcheck.h"

WBImportCFF::WBImportCFF(QObject *parent)
    : WBDocumentBasedImportAdaptor(parent)
{
    // NOOP
}


WBImportCFF::~WBImportCFF()
{
    // NOOP
}


QStringList WBImportCFF::supportedExtentions()
{
    return QStringList("iwb");
}


QString WBImportCFF::importFileFilter()
{
    QString filter = tr("Common File Format (");
    QStringList formats = supportedExtentions();
    bool isFirst = true;

    foreach(QString format, formats)
    {
            if(isFirst)
                    isFirst = false;
            else
                    filter.append(" ");

        filter.append("*."+format);
    }

    filter.append(")");

    return filter;
}

bool WBImportCFF::addFileToDocument(WBDocumentProxy* pDocument, const QFile& pFile)
{
    QFileInfo fi(pFile);
    WBApplication::showMessage(tr("Importing file %1...").arg(fi.baseName()), true);

    // first unzip the file to the correct place
    //TODO create temporary path for iwb file content
    QString path = QDir::tempPath();

    QString documentRootFolder = expandFileToDir(pFile, path);
        QString contentFile;
    if (documentRootFolder.isEmpty()) //if file has failed to unzip it is probably just xml file
        contentFile = pFile.fileName();
    else //get path to content xml (according to iwbcff specification)
        contentFile = documentRootFolder.append("/content.xml");

    if(!contentFile.length()){
            WBApplication::showMessage(tr("Import of file %1 failed.").arg(fi.baseName()));
            return false;
    }
    else{
        //TODO convert expanded CFF file content to the destination document
        //create destination document proxy
        //fill metadata and save
        WBDocumentProxy* destDocument = new WBDocumentProxy(WBPersistenceManager::persistenceManager()->generateUniqueDocumentPath());
        QDir dir;
        dir.mkdir(destDocument->persistencePath());

        //try to import cff to document
        if (WBCFFSubsetAdaptor::ConvertCFFFileToWbz(contentFile, destDocument))
        {
            WBPersistenceManager::persistenceManager()->addDirectoryContentToDocument(destDocument->persistencePath(), pDocument);
            WBFileSystemUtils::deleteDir(destDocument->persistencePath());
            delete destDocument;
            WBApplication::showMessage(tr("Import successful."));
            return true;
        }
        else
        {
            WBFileSystemUtils::deleteDir(destDocument->persistencePath());
            delete destDocument;
            WBApplication::showMessage(tr("Import failed."));
            return false;
        }
    }
}

QString WBImportCFF::expandFileToDir(const QFile& pZipFile, const QString& pDir)
{
    QuaZip zip(pZipFile.fileName());

    if(!zip.open(QuaZip::mdUnzip)) {
        qWarning() << "Import failed. Cause zip.open(): " << zip.getZipError();
        return "";
    }

    zip.setFileNameCodec("UTF-8");
    QuaZipFileInfo info;
    QuaZipFile file(&zip);

    //create unique cff document root fodler
    //use current date/time and temp number for folder name
    QString documentRootFolder;
    int tmpNumber = 0;
    QDir rootDir;
    while (true) {
        QString tempPath = QString("%1/sank%2.%3")
                .arg(pDir)
                .arg(QDateTime::currentDateTime().toString("dd_MM_yyyy_HH-mm"))
                .arg(tmpNumber);
        if (!rootDir.exists(tempPath)) {
            documentRootFolder = tempPath;
            break;
        }
        tmpNumber++;
        if (tmpNumber == 100000) {
            qWarning() << "Import failed. Failed to create temporary directory for iwb file";
            return "";
        }
    }
    if (!rootDir.mkdir(documentRootFolder)) {
        qWarning() << "Import failed. Couse: failed to create temp folder for cff package";
    }

    QFile out;
    char c;
    for(bool more=zip.goToFirstFile(); more; more=zip.goToNextFile()) {
        if(!zip.getCurrentFileInfo(&info)) {
            qWarning() << "Import failed. Cause: getCurrentFileInfo(): " << zip.getZipError();
            return "";
        }
//        if(!file.open(QIODevice::ReadOnly)) {
//            qWarning() << "Import failed. Cause: file.open(): " << zip.getZipError();
//            return "";
//        }
        file.open(QIODevice::ReadOnly);
        if(file.getZipError()!= UNZ_OK) {
            qWarning() << "Import failed. Cause: file.getFileName(): " << zip.getZipError();
            return "";
        }

        QString newFileName = documentRootFolder + "/" + file.getActualFileName();

        QFileInfo newFileInfo(newFileName);
        rootDir.mkpath(newFileInfo.absolutePath());

        out.setFileName(newFileName);
        out.open(QIODevice::WriteOnly);

        while(file.getChar(&c))
            out.putChar(c);

        out.close();

        if(file.getZipError()!=UNZ_OK) {
            qWarning() << "Import failed. Cause: " << zip.getZipError();
            return "";
        }
        if(!file.atEnd()) {
            qWarning() << "Import failed. Cause: read all but not EOF";
            return "";
        }

        file.close();

        if(file.getZipError()!=UNZ_OK) {
            qWarning() << "Import failed. Cause: file.close(): " <<  file.getZipError();
            return "";
        }
    }

    zip.close();

    if(zip.getZipError()!=UNZ_OK) {
        qWarning() << "Import failed. Cause: zip.close(): " << zip.getZipError();
        return "";
    }

    return documentRootFolder;
}


WBDocumentProxy* WBImportCFF::importFile(const QFile& pFile, const QString& pGroup)
{
    Q_UNUSED(pGroup); // group is defined in the imported file

    QFileInfo fi(pFile);
    WBApplication::showMessage(tr("Importing file %1...").arg(fi.baseName()), true);

    // first unzip the file to the correct place
    //TODO create temporary path for iwb file content
    QString path = QDir::tempPath();

    QString documentRootFolder = expandFileToDir(pFile, path);
    QString contentFile;
    if (documentRootFolder.isEmpty())
        //if file has failed to umzip it is probably just xml file
        contentFile = pFile.fileName();
    else
        //get path to content xml
        contentFile = QString("%1/content.xml").arg(documentRootFolder);

    if(!contentFile.length()){
            WBApplication::showMessage(tr("Import of file %1 failed.").arg(fi.baseName()));
            return 0;
    }
    else{
        //create destination document proxy
        //fill metadata and save
        WBDocumentProxy* destDocument = new WBDocumentProxy(WBPersistenceManager::persistenceManager()->generateUniqueDocumentPath());
        QDir dir;
        dir.mkdir(destDocument->persistencePath());
        if (pGroup.length() > 0)
            destDocument->setMetaData(WBSettings::documentGroupName, pGroup);
        if (fi.baseName() > 0)
            destDocument->setMetaData(WBSettings::documentName, fi.baseName());

        destDocument->setMetaData(WBSettings::documentVersion, WBSettings::currentFileVersion);
        destDocument->setMetaData(WBSettings::documentUpdatedAt, WBStringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));

        WBDocumentProxy* newDocument = NULL;
        //try to import cff to document
        if (WBCFFSubsetAdaptor::ConvertCFFFileToWbz(contentFile, destDocument))
        {
            newDocument = WBPersistenceManager::persistenceManager()->createDocumentFromDir(destDocument->persistencePath()
                                                                                            ,""
                                                                                            ,""
                                                                                            ,false
                                                                                            ,false
                                                                                            ,true);

            WBApplication::showMessage(tr("Import successful."));
        }
        else
        {
            WBFileSystemUtils::deleteDir(destDocument->persistencePath());
            WBApplication::showMessage(tr("Import failed."));
        }
        delete destDocument;

        if (documentRootFolder.length() != 0)
            WBFileSystemUtils::deleteDir(documentRootFolder);
        return newDocument;
    }
}
