#include "WBImportDocument.h"
#include "document/WBDocumentProxy.h"

#include "frameworks/WBFileSystemUtils.h"

#include "core/WBApplication.h"
#include "core/WBSettings.h"
#include "core/WBPersistenceManager.h"

#include "globals/WBGlobals.h"

THIRD_PARTY_WARNINGS_DISABLE
#include "quazip.h"
#include "quazipfile.h"
#include "quazipfileinfo.h"
THIRD_PARTY_WARNINGS_ENABLE

#include "core/memcheck.h"

WBImportDocument::WBImportDocument(QObject *parent)
    :WBDocumentBasedImportAdaptor(parent)
{
    // NOOP
}

WBImportDocument::~WBImportDocument()
{
    // NOOP
}


QStringList WBImportDocument::supportedExtentions()
{
    return QStringList("ubz");
}


QString WBImportDocument::importFileFilter()
{
    return tr("WBoard (*.ubz)");
}


bool WBImportDocument::extractFileToDir(const QFile& pZipFile, const QString& pDir, QString& documentRoot)
{
    QDir rootDir(pDir);
    QuaZip zip(pZipFile.fileName());

    if(!zip.open(QuaZip::mdUnzip))
    {
        qWarning() << "Import failed. Cause zip.open(): " << zip.getZipError();
        return false;
    }

    zip.setFileNameCodec("UTF-8");
    QuaZipFileInfo info;
    QuaZipFile file(&zip);

    QFile out;
    char c;
    documentRoot = WBPersistenceManager::persistenceManager()->generateUniqueDocumentPath(pDir);
    for(bool more=zip.goToFirstFile(); more; more=zip.goToNextFile())
    {
        if(!zip.getCurrentFileInfo(&info))
        {
            qWarning() << "Import failed. Cause: getCurrentFileInfo(): " << zip.getZipError();
            return false;
        }

        if(!file.open(QIODevice::ReadOnly))
        {
            qWarning() << "Import failed. Cause: file.open(): " << zip.getZipError();
            return false;
        }

        if(file.getZipError()!= UNZ_OK)
        {
            qWarning() << "Import failed. Cause: file.getFileName(): " << zip.getZipError();
            return false;
        }

        QString newFileName = documentRoot + "/" + file.getActualFileName();
        QFileInfo newFileInfo(newFileName);
        if (!rootDir.mkpath(newFileInfo.absolutePath()))
            return false;

        out.setFileName(newFileName);
        if (!out.open(QIODevice::WriteOnly))
            return false;

 
        QByteArray outFileContent = file.readAll();
        if (out.write(outFileContent) == -1)
        {
            qWarning() << "Import failed. Cause: Unable to write file";
            out.close();
            return false;
        }

        while(file.getChar(&c))
            out.putChar(c);

        out.close();

        if(file.getZipError()!=UNZ_OK)
        {
            qWarning() << "Import failed. Cause: " << zip.getZipError();
            return false;
        }

        if(!file.atEnd())
        {
            qWarning() << "Import failed. Cause: read all but not EOF";
            return false;
        }

        file.close();

        if(file.getZipError()!=UNZ_OK)
        {
            qWarning() << "Import failed. Cause: file.close(): " <<  file.getZipError();
            return false;
        }

    }

    zip.close();

    if(zip.getZipError()!=UNZ_OK)
    {
      qWarning() << "Import failed. Cause: zip.close(): " << zip.getZipError();
      return false;
    }

    return true;
}

WBDocumentProxy* WBImportDocument::importFile(const QFile& pFile, const QString& pGroup)
{
    Q_UNUSED(pGroup); 

    QFileInfo fi(pFile);
    WBApplication::showMessage(tr("Importing file %1...").arg(fi.baseName()), true);

    QString path = WBSettings::userDocumentDirectory();

    QString documentRootFolder;

    if(!extractFileToDir(pFile, path, documentRootFolder)){
        WBApplication::showMessage(tr("Import of file %1 failed.").arg(fi.baseName()));
        return NULL;
    }

    WBDocumentProxy* newDocument = WBPersistenceManager::persistenceManager()->createDocumentFromDir(documentRootFolder, pGroup, "", false, false, true);
    WBApplication::showMessage(tr("Import successful."));
    return newDocument;
}

bool WBImportDocument::addFileToDocument(WBDocumentProxy* pDocument, const QFile& pFile)
{
    QFileInfo fi(pFile);
    WBApplication::showMessage(tr("Importing file %1...").arg(fi.baseName()), true);

    QString path = WBFileSystemUtils::createTempDir();

    QString documentRootFolder;
    if (!extractFileToDir(pFile, path, documentRootFolder))
    {
        WBApplication::showMessage(tr("Import of file %1 failed.").arg(fi.baseName()));
        return false;
    }

    if (!WBPersistenceManager::persistenceManager()->addDirectoryContentToDocument(documentRootFolder, pDocument))
    {
        WBApplication::showMessage(tr("Import of file %1 failed.").arg(fi.baseName()));
        return false;
    }

    WBFileSystemUtils::deleteDir(path);

    WBApplication::showMessage(tr("Import successful."));

    return true;
}


