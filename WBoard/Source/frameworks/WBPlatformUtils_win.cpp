#include "WBPlatformUtils.h"

#include <QtWidgets>
#include <QProcess>
#include <qt_windows.h>
#include <shellapi.h>

#include "frameworks/WBFileSystemUtils.h"
#include "core/memcheck.h"

void WBPlatformUtils::init()
{
    initializeKeyboardLayouts();
}


QString WBPlatformUtils::applicationResourcesDirectory()
{
    return QApplication::applicationDirPath();
}


void WBPlatformUtils::hideFile(const QString &filePath)
{
    Q_UNUSED(filePath);

}

void WBPlatformUtils::setFileType(const QString &filePath, unsigned long fileType)
{
    Q_UNUSED(filePath);
    Q_UNUSED(fileType);

}

void WBPlatformUtils::fadeDisplayOut()
{
    // NOOP
}

void WBPlatformUtils::fadeDisplayIn()
{
    // NOOP
}

QStringList WBPlatformUtils::availableTranslations()
{
    QString translationsPath = applicationResourcesDirectory() + "/" + "Language" + "/";
    QStringList translationsList = WBFileSystemUtils::allFiles(translationsPath);
    QRegExp sankoreTranslationFiles(".*WBoard_.*.qm");
    translationsList=translationsList.filter(sankoreTranslationFiles);
    return translationsList.replaceInStrings(QRegExp("(.*)WBoard_(.*).qm"),"\\2");
}

QString WBPlatformUtils::translationPath(QString pFilePrefix,QString pLanguage)
{
    QString qmPath = applicationResourcesDirectory() + "/" + "Language" + "/" + pFilePrefix + pLanguage + ".qm";
    return qmPath;
}

QString WBPlatformUtils::systemLanguage()
{
    return QLocale::system().name();
}

void WBPlatformUtils::bringPreviousProcessToFront()
{
    // Mac only
}


QString WBPlatformUtils::osUserLoginName()
{
    WCHAR winUserName[256 + 1];
    DWORD winUserNameSize = sizeof(winUserName);
    GetUserName( winUserName, &winUserNameSize );
    QString userName = QString::fromWCharArray(winUserName, winUserNameSize - 1);

    return userName;
}


QString WBPlatformUtils::computerName()
{
    WCHAR winComputerName[256 + 1];
    DWORD winComputerNameSize = sizeof(winComputerName);
    GetComputerName(winComputerName, &winComputerNameSize );
    QString computerName = QString::fromWCharArray(winComputerName, winComputerNameSize - 1);

    return computerName;
}


void WBPlatformUtils::setDesktopMode(bool desktop)
{
    Q_UNUSED(desktop);
}

void WBPlatformUtils::setWindowNonActivableFlag(QWidget* widget, bool nonAcivable)
{/*
    long exStyle = (nonAcivable) ? GetWindowLong(widget->winId(), GWL_EXSTYLE) | WS_EX_NOACTIVATE
        : GetWindowLong(widget->winId(), GWL_EXSTYLE) & ~WS_EX_NOACTIVATE;

    SetWindowLong(widget->winId(), GWL_EXSTYLE, exStyle);
    */
}

#define KEYBTDECL(s1, s2, clSwitch) KEYBT(s1, s2, clSwitch, 0, 0, KEYCODE(s1), KEYCODE(s2))

KEYBT ENGLISH_LOCALE[] = {
    /* ` ~ */ KEYBTDECL(0x60, 0x7e, false),
    /* 1 ! */ KEYBTDECL(0x31, 0x21, false),
    /* 2 @ */ KEYBTDECL(0x32, 0x40, false),
    /* 3 # */ KEYBTDECL(0x33, 0x23, false),
    /* 4 $ */ KEYBTDECL(0x34, 0x24, false),
    /* 5 % */ KEYBTDECL(0x35, 0x25, false),
    /* 6 ^ */ KEYBTDECL(0x36, 0x5e, false),
    /* 7 & */ KEYBTDECL(0x37, 0x26, false),
    /* 8 * */ KEYBTDECL(0x38, 0x2a, false),
    /* 9 ( */ KEYBTDECL(0x39, 0x28, false),
    /* 0 ) */ KEYBTDECL(0x30, 0x29, false),
    /* - _ */ KEYBTDECL(0x2d, 0x5f, false),
    /* = + */ KEYBTDECL(0x3d, 0x2b, false),

    /* q Q */ KEYBTDECL(0x71, 0x51, true),
    /* w W */ KEYBTDECL(0x77, 0x57, true),
    /* e E */ KEYBTDECL(0x65, 0x45, true),
    /* r R */ KEYBTDECL(0x72, 0x52, true),
    /* t T */ KEYBTDECL(0x74, 0x54, true),
    /* y Y */ KEYBTDECL(0x79, 0x59, true),
    /* u U */ KEYBTDECL(0x75, 0x55, true),
    /* i I */ KEYBTDECL(0x69, 0x49, true),
    /* o O */ KEYBTDECL(0x6f, 0x4f, true),
    /* p P */ KEYBTDECL(0x70, 0x50, true),
    /* [ { */ KEYBTDECL(0x5b, 0x7b, false),
    /* ] } */ KEYBTDECL(0x5d, 0x7d, false),

    /* a A */ KEYBTDECL(0x61, 0x41, true),
    /* s S */ KEYBTDECL(0x73, 0x53, true),
    /* d D */ KEYBTDECL(0x64, 0x44, true),
    /* f F */ KEYBTDECL(0x66, 0x46, true),
    /* g G */ KEYBTDECL(0x67, 0x47, true),
    /* h H */ KEYBTDECL(0x68, 0x48, true),
    /* j J */ KEYBTDECL(0x6a, 0x4a, true),
    /* k K */ KEYBTDECL(0x6b, 0x4b, true),
    /* l L */ KEYBTDECL(0x6c, 0x4c, true),
    /* ; : */ KEYBTDECL(0x3b, 0x3a, false),
    /* ' " */ KEYBTDECL(0x27, 0x22, false),
    /* \ | */ KEYBTDECL(0x5c, 0x7c, false),

    /* z Z */ KEYBTDECL(0x7a, 0x5a, true),
    /* x X */ KEYBTDECL(0x78, 0x58, true),
    /* c C */ KEYBTDECL(0x63, 0x43, true),
    /* v V */ KEYBTDECL(0x76, 0x56, true),
    /* b B */ KEYBTDECL(0x62, 0x42, true),
    /* n N */ KEYBTDECL(0x6e, 0x4e, true),
    /* m M */ KEYBTDECL(0x6d, 0x4d, true),
    /* , < */ KEYBTDECL(0x2c, 0x3c, false),
    /* . > */ KEYBTDECL(0x2e, 0x3e, false),
    /* / ? */ KEYBTDECL(0x2f, 0x5f, false)};


void WBPlatformUtils::initializeKeyboardLayouts()
{
    nKeyboardLayouts = 1;
    keyboardLayouts = new WBKeyboardLocale*[nKeyboardLayouts];
    keyboardLayouts[0] = new WBKeyboardLocale(tr("English"), "en", "", new QIcon(":/images/flags/en.png"), ENGLISH_LOCALE);
}

void WBPlatformUtils::destroyKeyboardLayouts()
{
    for(int i=0; i<nKeyboardLayouts; i++)
        delete keyboardLayouts[i];
    delete [] keyboardLayouts;
    keyboardLayouts = NULL;
}

QString WBPlatformUtils::urlFromClipboard()
{
    QString qsRet;
    //  Not implemented yet
    return qsRet;
}

void WBPlatformUtils::setFrontProcess()
{
    // not used in Windows
}


void WBPlatformUtils::showFullScreen(QWidget *pWidget)
{
    pWidget->showFullScreen();
}

void WBPlatformUtils::showOSK(bool show)
{
    if (show) {
        QString windir = qgetenv("WINDIR");
        QString osk_path = windir+"\\System32\\osk.exe";

        QProcess oskProcess;
        // We have to pass by explorer.exe because osk.exe can only be launched
        // directly with administrator rights
        oskProcess.startDetached("explorer.exe", QStringList() << osk_path);
    }

    else {
        HWND oskWindow = ::FindWindow(TEXT("OSKMainClass"), NULL);
        if (oskWindow)
            PostMessage(oskWindow, WM_SYSCOMMAND, SC_CLOSE, 0);
    }
}
