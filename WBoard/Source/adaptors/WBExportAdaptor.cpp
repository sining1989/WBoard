#include <QFileDialog>

#include "WBExportAdaptor.h"

#include "document/WBDocumentProxy.h"

#include "frameworks/WBFileSystemUtils.h"

#include "core/WBSettings.h"
#include "core/WBSetting.h"
#include "core/WBApplication.h"

#include "gui/WBMainWindow.h"
#include "gui/WBMessagesDialog.h"

#include "core/memcheck.h"

WBExportAdaptor::WBExportAdaptor(QObject *parent)
    : QObject(parent)
    , mIsVerbose(true)
{
    // NOOP
}


WBExportAdaptor::~WBExportAdaptor()
{
    // NOOP
}

QString WBExportAdaptor::askForFileName(WBDocumentProxy* pDocument, const QString& pDialogTitle)
{
    QString defaultName;

    defaultName += pDocument->metaData(WBSettings::documentName).toString() + exportExtention();

    defaultName = WBFileSystemUtils::cleanName(defaultName);

    QString defaultPath = WBSettings::settings()->lastExportFilePath->get().toString() + "/" + defaultName;

    QString filename = QFileDialog::getSaveFileName(WBApplication::mainWindow, pDialogTitle, defaultPath, QString());

    if (filename.size() == 0)
    {
        return filename;
    }

    // add extension if needed
    QFileInfo fileNameInfo(filename);
    if (fileNameInfo.suffix() != exportExtention().mid(1, exportExtention().length() - 1))
    {
        filename += exportExtention();
    }
    WBSettings::settings()->lastExportFilePath->set(QVariant(fileNameInfo.absolutePath()));
    QApplication::processEvents();

    return filename;
}

QString WBExportAdaptor::askForDirName(WBDocumentProxy* pDocument, const QString& pDialogTitle)
{
    QString defaultPath = WBSettings::settings()->lastExportDirPath->get().toString();

    QString container = QFileDialog::getExistingDirectory(WBApplication::mainWindow, pDialogTitle, defaultPath);

    QString dirname;

    if (container.size() > 0)
    {
        WBSettings::settings()->lastExportDirPath->set(QVariant(container));

        QString docname;

        if (pDocument->metaData(WBSettings::documentGroupName).toString().length() > 0)
        {
            docname += pDocument->metaData(WBSettings::documentGroupName).toString() + QString(" ");
        }

        docname += pDocument->metaData(WBSettings::documentName).toString() + exportExtention();
        docname = WBFileSystemUtils::cleanName(docname);

        dirname = container + "/" + docname;
    }

    QApplication::processEvents();

    return dirname;
}

void WBExportAdaptor::persistLocally(WBDocumentProxy* pDocumentProxy, const QString& pDialogTitle)
{
    if (!pDocumentProxy)
        return;

    QString filename = askForFileName(pDocumentProxy, pDialogTitle);

    if (filename.length() > 0) {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

        if (mIsVerbose)
            WBApplication::showMessage(tr("Exporting document..."));

        // Check that the location is writeable
        QFileInfo info(filename);
        info.setFile(info.absolutePath());

        if (!info.isWritable()) {
            QMessageBox errorBox;
            errorBox.setWindowTitle(tr("Export failed"));
            errorBox.setText(tr("Unable to export to the selected location. You do not have the permissions necessary to save the file."));
            errorBox.setIcon(QMessageBox::Critical);
            errorBox.exec();

            if (mIsVerbose)
                WBApplication::showMessage(tr("Export failed: location not writable"));

            QApplication::restoreOverrideCursor();

            return;
        }

        bool persisted = this->persistsDocument(pDocumentProxy, filename);

        if (mIsVerbose && persisted)
            WBApplication::showMessage(tr("Export successful."));

        QApplication::restoreOverrideCursor();
    }
}

bool WBExportAdaptor::persistsDocument(WBDocumentProxy* pDocument, const QString& filename)
{
    // Implemented in child classes

    Q_UNUSED(pDocument);
    Q_UNUSED(filename);
    return false;
}

void WBExportAdaptor::showErrorsList(QList<QString> errorsList)
{
    if (errorsList.count())
    {
        WBMessagesDialog *dialog = new WBMessagesDialog(tr("Warnings during export was appeared"), WBApplication::mainWindow);
        dialog->setMessages(errorsList);
        dialog->show();
    }
}
