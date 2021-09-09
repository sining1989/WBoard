#ifndef WBFILESYSTEMUTILS_H_
#define WBFILESYSTEMUTILS_H_

#include <QtCore>
#include <QThread>

#include "core/WB.h"

class QuaZipFile;
class WBProcessingProgressListener;

class WBFileSystemUtils : public QObject
{
    Q_OBJECT

    public:
        WBFileSystemUtils();
        virtual ~WBFileSystemUtils();

        static QString removeLocalFilePrefix(QString input);

        static QString defaultTempDirName() { return QCoreApplication::applicationName(); }
        static QString defaultTempDirPath();
        static QString createTempDir(const QString& templateString = defaultTempDirName(), bool autoDeleteOnExit = true);
        static void cleanupGhostTempFolders(const QString& templateString = defaultTempDirName());

        static void deleteAllTempDirCreatedDuringSession();

        static QFileInfoList allElementsInDirectory(const QString& pDirPath);

        static QStringList allFiles(const QString& pDirPath, const bool isRecurive=true);

        static bool deleteDir(const QString& pDirPath);

        static bool copyDir(const QString& pSourceDirPath, const QString& pTargetDirPath);

        static bool moveDir(const QString& pSourceDirPath, const QString& pTargetDirPath);

        static bool copyFile(const QString &source, const QString &destination, bool overwrite = false);

        static bool copy(const QString &source, const QString &Destination, bool overwrite = false);

        static QString cleanName(const QString& name);

        static QString digitFileFormat(const QString& s, int digit);

        static QString thumbnailPath(const QString& path);

        static QString mimeTypeFromFileName(const QString& filename);

        static QString fileExtensionFromMimeType(const QString& pMimeType);

        static WBMimeType::Enum mimeTypeFromString(const QString& typeString);

        static WBMimeType::Enum mimeTypeFromUrl(const QUrl& url);

        static QString normalizeFilePath(const QString& pFilePath);

        static QString extension(const QString& filaname);

        static QString lastPathComponent(const QString& path);

        static QString getFirstExistingFileFromList(const QString& path, const QStringList& files);

        static bool isAZipFile(QString &filePath);

        static bool deleteFile(const QString &path);

        static bool compressDirInZip(const QDir& pDir, const QString& pDestDir, QuaZipFile *pOutZipFile
                        , bool pRootDocumentFolder, WBProcessingProgressListener* progressListener = 0);

        static bool expandZipToDir(const QFile& pZipFile, const QDir& pTargetDir);

        static QString md5InHex(const QByteArray &pByteArray);
        static QString md5(const QByteArray &pByteArray);

        static QString nextAvailableFileName(const QString& filename, const QString& inter = QString(""));

        static QString readTextFile(QString path);

    private:
        static QStringList sTempDirToCleanUp;

};

class WBProcessingProgressListener
{
public:
    WBProcessingProgressListener()
    {

    }

    virtual ~WBProcessingProgressListener()
    {

    }

    virtual void processing(const QString& pOpType, int pCurrent, int pTotal) = 0;

};

#endif /* WBFILESYSTEMUTILS_H_ */
