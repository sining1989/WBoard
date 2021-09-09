#include <QtWidgets>
#include <QToolTip>
#include <QStackedLayout>

#include "WBMainWindow.h"
#include "core/WBApplication.h"
#include "core/WBApplicationController.h"
#include "board/WBBoardController.h"
#include "core/WBDisplayManager.h"

// work around for handling tablet events on MAC OS with Qt 4.8.0 and above
#if defined(Q_OS_OSX)
#include "board/WBBoardView.h"
#endif

#include "core/memcheck.h"

WBMainWindow::WBMainWindow(QWidget *parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags)
    , mBoardWidget(0)
    , mWebWidget(0)
    , mDocumentsWidget(0)
    , mpDownloadWidget(NULL)
{
    Ui::MainWindow::setupUi(this);

    mpDownloadWidget = new WBDownloadWidget();
    mpDownloadWidget->setWindowModality(Qt::ApplicationModal);

    //Setting tooltip colors staticly, since they look not quite well on different color themes
    QPalette toolTipPalette;
    toolTipPalette.setColor(QPalette::ToolTipBase, QColor("#FFFFDC"));
    toolTipPalette.setColor(QPalette::ToolTipText, Qt::black);
    QToolTip::setPalette(toolTipPalette);

    QWidget* centralWidget = new QWidget(this);
	centralWidget->setMinimumWidth(200);
    mStackedLayout = new QStackedLayout(centralWidget);
    setCentralWidget(centralWidget);

#ifdef Q_OS_OSX
    actionPreferences->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Comma));
    actionQuit->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));
#elif defined(Q_OS_WIN)
    actionPreferences->setShortcut(QKeySequence(Qt::ALT + Qt::Key_Return));
    // this code, because it unusable, system key combination can`t be triggered, even we add it manually
    actionQuit->setShortcut(QKeySequence(Qt::ALT + Qt::Key_F4));
#else
    actionQuit->setShortcut(QKeySequence(Qt::ALT + Qt::Key_F4));
#endif
}

WBMainWindow::~WBMainWindow()
{
    if(NULL != mpDownloadWidget)
    {
        delete mpDownloadWidget;
        mpDownloadWidget = NULL;
    }
}

void WBMainWindow::addBoardWidget(QWidget *pWidget)
{
    if (!mBoardWidget)
    {
        mBoardWidget = pWidget;
        mStackedLayout->addWidget(mBoardWidget);
    }
}

void WBMainWindow::switchToBoardWidget()
{
    if (mBoardWidget)
    {
        mStackedLayout->setCurrentWidget(mBoardWidget);
    }
}

void WBMainWindow::addWebWidget(QWidget *pWidget)
{
    if (!mWebWidget)
    {
        mWebWidget = pWidget;
        mStackedLayout->addWidget(mWebWidget);
    }
}

void WBMainWindow::switchToWebWidget()
{
    qDebug() << "popped out from StackedLayout size height: " << mWebWidget->height() << " width: " << mWebWidget->width();
    if (mWebWidget)
    {
        mStackedLayout->setCurrentWidget(mWebWidget);
    }
}


void WBMainWindow::addDocumentsWidget(QWidget *pWidget)
{
    if (!mDocumentsWidget)
    {
        mDocumentsWidget = pWidget;
        mStackedLayout->addWidget(mDocumentsWidget);
    }
}

void WBMainWindow::switchToDocumentsWidget()
{
    if (mDocumentsWidget)
    {
        mStackedLayout->setCurrentWidget(mDocumentsWidget);
    }
}

void WBMainWindow::keyPressEvent(QKeyEvent *event)
{
    QMainWindow::keyPressEvent(event);
}

void WBMainWindow::closeEvent(QCloseEvent *event)
{
    event->ignore();
    emit closeEvent_Signal(event);
}

// work around for handling tablet events on MAC OS with Qt 4.8.0 and above
#if defined(Q_OS_OSX)
bool WBMainWindow::event(QEvent *event)
{
    bool bRes = QMainWindow::event(event);

    if (NULL != WBApplication::boardController)
    {
        WBBoardView *controlV = WBApplication::boardController->controlView();
        if (controlV && controlV->isVisible())
        {
            switch (event->type())
            {
            case QEvent::TabletEnterProximity:
            case QEvent::TabletLeaveProximity:
            case QEvent::TabletMove:
            case QEvent::TabletPress:
            case QEvent::TabletRelease:
                {
                    return controlV->directTabletEvent(event);
                }
            }
        }
    }
    return bRes;
}
#endif

void WBMainWindow::onExportDone()
{
    // HACK :  When opening the file save dialog during the document exportation,
    //         some buttons of the toolbar become disabled without any reason. We
    //         re-enable them here.
    actionExport->setEnabled(true);
    actionNewDocument->setEnabled(true);
    actionRename->setEnabled(true);
    actionDuplicate->setEnabled(true);
    actionDelete->setEnabled(true);
    actionOpen->setEnabled(true);
    actionDocumentAdd->setEnabled(true);
}

bool WBMainWindow::yesNoQuestion(QString windowTitle, QString text)
{
    QMessageBox messageBox;
    messageBox.setParent(this);
    messageBox.setWindowTitle(windowTitle);
    messageBox.setText(text);
    QPushButton* yesButton = messageBox.addButton(tr("Yes"),QMessageBox::YesRole);
    messageBox.addButton(tr("No"),QMessageBox::NoRole);
    messageBox.setIcon(QMessageBox::Question);

#ifdef Q_OS_LINUX
    // to avoid to be handled by x11. This allows us to keep to the back all the windows manager stuff like palette, toolbar ...
    messageBox.setWindowFlags(Qt::Dialog | Qt::X11BypassWindowManagerHint);
#else
    messageBox.setWindowFlags(Qt::Dialog);
#endif

    messageBox.exec();
    return messageBox.clickedButton() == yesButton;
}

void WBMainWindow::oneButtonMessageBox(QString windowTitle, QString text, QMessageBox::Icon type)
{
    QMessageBox messageBox;
    messageBox.setParent(this);
    messageBox.setWindowFlags(Qt::Dialog);
    messageBox.setWindowTitle(windowTitle);
    messageBox.setText(text);
    messageBox.addButton(tr("Ok"),QMessageBox::YesRole);
    messageBox.setIcon(type);
    messageBox.exec();
}

void WBMainWindow::warning(QString windowTitle, QString text)
{
    oneButtonMessageBox(windowTitle,text, QMessageBox::Warning);
}

void WBMainWindow::information(QString windowTitle, QString text)
{
    oneButtonMessageBox(windowTitle, text, QMessageBox::Information);
}

void WBMainWindow::showDownloadWidget()
{
    if(NULL != mpDownloadWidget)
    {
        mpDownloadWidget->show();
    }
}

void WBMainWindow::hideDownloadWidget()
{
    if(NULL != mpDownloadWidget)
    {
        mpDownloadWidget->hide();
    }
}
