#ifndef WBDOCUMENTMANAGER_H_
#define WBDOCUMENTMANAGER_H_

#include <QtCore>

class WBExportAdaptor;
class WBImportAdaptor;
class WBDocumentProxy;

class WBDocumentManager : public QObject
{
    Q_OBJECT

    public:
        static WBDocumentManager* documentManager();
        virtual ~WBDocumentManager();


        QString importFileFilter(bool notUbx = false);

        QStringList importFileExtensions(bool notUbx = false);

        QFileInfoList importUbx(const QString &Incomingfile, const QString &destination);
        WBDocumentProxy* importFile(const QFile& pFile, const QString& pGroup);

        int addFilesToDocument(WBDocumentProxy* pDocument, QStringList fileNames);

        WBDocumentProxy* importDir(const QDir& pDir, const QString& pGroup);
        int addImageDirToDocument(const QDir& pDir, WBDocumentProxy* pDocument);

        QList<WBExportAdaptor*> supportedExportAdaptors();
        void emitDocumentUpdated(WBDocumentProxy* pDocument);

    signals:
        void documentUpdated(WBDocumentProxy *pDocument);

    private:
        WBDocumentManager(QObject *parent = 0);
        QList<WBExportAdaptor*> mExportAdaptors;
        QList<WBImportAdaptor*> mImportAdaptors;

        static WBDocumentManager* sDocumentManager;
};

#endif /* WBDOCUMENTMANAGER_H_ */
