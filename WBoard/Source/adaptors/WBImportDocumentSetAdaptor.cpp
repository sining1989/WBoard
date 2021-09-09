#include "WBImportDocumentSetAdaptor.h"

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

WBImportDocumentSetAdaptor::WBImportDocumentSetAdaptor(QObject *parent)
    :WBImportAdaptor(parent)
{
    // NOOP
}

WBImportDocumentSetAdaptor::~WBImportDocumentSetAdaptor()
{
    // NOOP
}


QStringList WBImportDocumentSetAdaptor::supportedExtentions()
{
    return QStringList("ubx");
}


QString WBImportDocumentSetAdaptor::importFileFilter()
{
    return tr("Wboard (set of documents) (*.ubx)");
}

QFileInfoList WBImportDocumentSetAdaptor::importData(const QString &zipFile, const QString &destination)
{
    QString tmpDir;
    int i = 0;
    QFileInfoList result;
    do {
      tmpDir = QDir::tempPath() + "/Sankore_tmpImportUBX_" + QString::number(i++);
    } while (QFileInfo(tmpDir).exists());

    QDir(QDir::tempPath()).mkdir(tmpDir);

    QFile fZipFile(zipFile);

    if (!extractFileToDir(fZipFile, tmpDir)) {
        WBFileSystemUtils::deleteDir(tmpDir);
        return QFileInfoList();
    }

    QDir tDir(tmpDir);

    foreach(QFileInfo readDir, tDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden , QDir::Name)) {
        QString newFileName = readDir.fileName();
        if (QFileInfo(destination + "/" + readDir.fileName()).exists()) {
            newFileName = QFileInfo(WBPersistenceManager::persistenceManager()->generateUniqueDocumentPath(tmpDir)).fileName();
        }
        QString newFilePath = destination + "/" + newFileName;
        if (WBFileSystemUtils::copy(readDir.absoluteFilePath(), newFilePath)) {
            result.append(newFilePath);
        }
    }

    WBFileSystemUtils::deleteDir(tmpDir);

    return result;
}


bool WBImportDocumentSetAdaptor::extractFileToDir(const QFile& pZipFile, const QString& pDir)
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

    QString documentRoot = QFileInfo(pDir).absoluteFilePath();
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

        QString actFileName = file.getActualFileName();
//        int ind = actFileName.indexOf("/");
//        if ( ind!= -1) {
//            actFileName.remove(0, ind + 1);
//        }
        QString newFileName = documentRoot + "/" + actFileName;
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
