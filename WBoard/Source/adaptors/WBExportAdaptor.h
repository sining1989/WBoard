#ifndef WBEXPORTADAPTOR_H_
#define WBEXPORTADAPTOR_H_

#include <QtWidgets>

class WBDocumentProxy;

class WBExportAdaptor : public QObject
{
    Q_OBJECT

    public:
        WBExportAdaptor(QObject *parent = 0);
        virtual ~WBExportAdaptor();

        virtual QString exportName() = 0;
        virtual QString exportExtention() { return "";}
        virtual void persist(WBDocumentProxy* pDocument) = 0;
        virtual bool persistsDocument(WBDocumentProxy* pDocument, const QString& filename);
        virtual bool associatedActionactionAvailableFor(const QModelIndex &selectedIndex) {Q_UNUSED(selectedIndex); return false;}
        QAction *associatedAction() {return mAssociatedAction;}
        void setAssociatedAction(QAction *pAssociatedAction) {mAssociatedAction = pAssociatedAction;}

        virtual void setVerbose(bool verbose)
        {
            mIsVerbose = verbose;
        }

        virtual bool isVerbose() const
        {
            return mIsVerbose;
        }

    protected:
        QString askForFileName(WBDocumentProxy* pDocument, const QString& pDialogTitle);
        QString askForDirName(WBDocumentProxy* pDocument, const QString& pDialogTitle);

        virtual void persistLocally(WBDocumentProxy* pDocumentProxy, const QString &pDialogTitle);

        void showErrorsList(QList<QString> errorsList);

        bool mIsVerbose;
        QAction* mAssociatedAction;

};

#endif /* WBEXPORTADAPTOR_H_ */
