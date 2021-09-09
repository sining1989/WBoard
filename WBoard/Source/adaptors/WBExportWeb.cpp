#include "WBExportWeb.h"

#include "frameworks/WBPlatformUtils.h"
#include "frameworks/WBFileSystemUtils.h"

#include "core/WBDocumentManager.h"
#include "core/WBApplication.h"

#include "document/WBDocumentProxy.h"

#include "globals/WBGlobals.h"

THIRD_PARTY_WARNINGS_DISABLE
#include "quazip.h"
#include "quazipfile.h"
THIRD_PARTY_WARNINGS_ENABLE

#include "core/memcheck.h"

WBExportWeb::WBExportWeb(QObject *parent)
    : WBExportAdaptor(parent)
{
    WBExportWeb::tr("Page"); // dummy slot for translation
}


WBExportWeb::~WBExportWeb()
{
    // NOOP
}


void WBExportWeb::persist(WBDocumentProxy* pDocumentProxy)
{
    if (!pDocumentProxy)
        return;

    QString dirName = askForDirName(pDocumentProxy, tr("Export as Web data"));

    if (dirName.length() > 0)
    {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        WBApplication::showMessage(tr("Exporting document..."));

        if(WBFileSystemUtils::copyDir(pDocumentProxy->persistencePath(), dirName))
        {
            QString htmlPath = dirName + "/index.html";

            QFile html(":www/WBoard-web-player.html");
            html.copy(htmlPath);

            WBApplication::showMessage(tr("Export successful."));

            QDesktopServices::openUrl(QUrl::fromLocalFile(htmlPath));
        }
        else
        {
            WBApplication::showMessage(tr("Export failed."));
        }


        QApplication::restoreOverrideCursor();
    }
}


QString WBExportWeb::exportName()
{
    return tr("Export to Web Browser");
}
