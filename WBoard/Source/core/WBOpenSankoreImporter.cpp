#include <QProcess>

#include "WBOpenSankoreImporter.h"
#include "WBSettings.h"

#include "core/WBApplication.h"
#include "gui/WBMainWindow.h"
#include "gui/WBOpenSankoreImporterWidget.h"

WBOpenSankoreImporter::WBOpenSankoreImporter(QWidget* mainWidget, QObject *parent) :
    QObject(parent)
  , mImporterWidget(NULL)
{
    if(WBSettings::settings()->appLookForOpenSankoreInstall->get().toBool() &&
            QDir(WBSettings::userDataDirectory().replace(qApp->applicationName(),"Sankore")).exists()){

        mImporterWidget = new WBOpenSankoreImporterWidget(mainWidget);

        connect(mImporterWidget->proceedButton(),SIGNAL(clicked()),mImporterWidget,SLOT(close()));
        connect(mImporterWidget->proceedButton(),SIGNAL(clicked()),this,SLOT(onProceedClicked()));
    }
}

void WBOpenSankoreImporter::onProceedClicked()
{
    QProcess newProcess;
#ifdef Q_OS_LINUX
    newProcess.startDetached(qApp->applicationDirPath()+"/importer/OpenBoardImporter");
#elif defined Q_OS_OSX
    newProcess.startDetached(qApp->applicationDirPath()+"/../Resources/OpenBoardImporter.app/Contents/MacOS/OpenBoardImporter");
#elif defined Q_OS_WIN
    QString importerPath = QDir::toNativeSeparators(qApp->applicationDirPath())+"\\OpenBoardImporter.exe";
    newProcess.startDetached("explorer.exe", QStringList() << importerPath);
#endif
    qApp->exit(0);

}


