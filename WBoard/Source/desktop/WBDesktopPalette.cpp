#include "WBDesktopPalette.h"

#include <QtWidgets>

#include "frameworks/WBPlatformUtils.h"

#include "core/WBSettings.h"
#include "core/WBSetting.h"
#include "core/WBApplication.h"
#include "core/WBApplicationController.h"

#include "board/WBDrawingController.h"

#include "gui/WBMainWindow.h"

#include "core/memcheck.h"

WBDesktopPalette::WBDesktopPalette(QWidget *parent, WBRightPalette* _rightPalette)
    : WBActionPalette(Qt::TopLeftCorner, parent)
    , mShowHideAction(NULL)
    , mDisplaySelectAction(NULL)
    , rightPalette(_rightPalette)
{
    QList<QAction*> actions;

	QIcon uniboardIcon;
	uniboardIcon.addPixmap(QPixmap(":/images/toolbar/board.png"), QIcon::Normal, QIcon::Off);
	uniboardIcon.addPixmap(QPixmap(":/images/toolbar/boardOn.png"), QIcon::Normal, QIcon::On);
    mActionUniboard = new QAction(QIcon(":/images/toolbar/board.png"), tr("Show WBoard"), this);
    connect(mActionUniboard, SIGNAL(triggered()), this, SIGNAL(uniboardClick()));
    actions << mActionUniboard;

    actions << WBApplication::mainWindow->actionPen;
    actions << WBApplication::mainWindow->actionEraser;
    actions << WBApplication::mainWindow->actionMarker;
    actions << WBApplication::mainWindow->actionSelector;
    actions << WBApplication::mainWindow->actionPointer;

    if (WBPlatformUtils::hasVirtualKeyboard())
        actions << WBApplication::mainWindow->actionVirtualKeyboard;

	QIcon actionCustomIcon;
	actionCustomIcon.addPixmap(QPixmap(":/images/toolbar/captureScreen.png"), QIcon::Normal, QIcon::Off);
	actionCustomIcon.addPixmap(QPixmap(":/images/toolbar/captureScreenOn.png"), QIcon::Normal, QIcon::On);
    mActionCustomSelect = new QAction(QIcon(":/images/toolbar/captureScreen.png"), tr("Capture Part of the Screen"), this);
    connect(mActionCustomSelect, SIGNAL(triggered()), this, SIGNAL(customClick()));
    actions << mActionCustomSelect;

	QIcon displaySelectIcon;
	displaySelectIcon.addPixmap(QPixmap(":/images/toolbar/captureArea.png"), QIcon::Normal, QIcon::Off);
	displaySelectIcon.addPixmap(QPixmap(":/images/toolbar/captureAreaOn.png"), QIcon::Normal, QIcon::On);
    mDisplaySelectAction = new QAction(displaySelectIcon, tr("Capture the Screen"), this);
    connect(mDisplaySelectAction, SIGNAL(triggered()), this, SIGNAL(screenClick()));
    actions << mDisplaySelectAction;

    QIcon showHideIcon;
    showHideIcon.addPixmap(QPixmap(":/images/toolbar/eyeOpened.png"), QIcon::Normal , QIcon::On);
    showHideIcon.addPixmap(QPixmap(":/images/toolbar/eyeClosed.png"), QIcon::Normal , QIcon::Off);
    mShowHideAction = new QAction(showHideIcon, "", this);
    mShowHideAction->setCheckable(true);

    connect(mShowHideAction, SIGNAL(triggered(bool)), this, SLOT(showHideClick(bool)));
    actions << mShowHideAction;

    setActions(actions);
    setButtonIconSize(QSize(42, 42));

    adjustSizeAndPosition();

    //  This palette can be minimized
    QIcon maximizeIcon;
    maximizeIcon.addPixmap(QPixmap(":/images/toolbar/stylusTab.png"), QIcon::Normal, QIcon::On);
    mMaximizeAction = new QAction(maximizeIcon, tr("Show the stylus palette"), this);
    connect(mMaximizeAction, SIGNAL(triggered()), this, SLOT(maximizeMe()));
    connect(this, SIGNAL(maximizeStart()), this, SLOT(maximizeMe()));
    connect(this, SIGNAL(minimizeStart(eMinimizedLocation)), this, SLOT(minimizeMe(eMinimizedLocation)));
    setMinimizePermission(true);

    connect(rightPalette, SIGNAL(resized()), this, SLOT(parentResized()));
}

WBDesktopPalette::~WBDesktopPalette()
{

}

void WBDesktopPalette::adjustPosition()
{
    QPoint pos = this->pos();
    if(this->pos().y() < 30){
        pos.setY(30);
        moveInsideParent(pos);
    }
}

void WBDesktopPalette::disappearForCapture()
{
    setWindowOpacity(0.0);
    qApp->processEvents();
}


void WBDesktopPalette::appear()
{
    setWindowOpacity(1.0);
}


void WBDesktopPalette::showHideClick(bool checked)
{
    WBApplication::applicationController->mirroringEnabled(checked);
}


void WBDesktopPalette::updateShowHideState(bool pShowEnabled)
{
    if (mShowHideAction)
        mShowHideAction->setChecked(pShowEnabled);

    if (mShowHideAction->isChecked())
        mShowHideAction->setToolTip(tr("Show Board on Secondary Screen"));
    else
        mShowHideAction->setToolTip(tr("Show Desktop on Secondary Screen"));

    if (pShowEnabled)
        raise();
}


void WBDesktopPalette::setShowHideButtonVisible(bool visible)
{
    mShowHideAction->setVisible(visible);
}


void WBDesktopPalette::setDisplaySelectButtonVisible(bool visible)
{
    mDisplaySelectAction->setVisible(visible);
}

//  Called when the palette is near the border and must be minimized
void WBDesktopPalette::minimizeMe(eMinimizedLocation location)
{
    Q_UNUSED(location);
    QList<QAction*> actions;
    clearLayout();

    actions << mMaximizeAction;
    setActions(actions);

    adjustSizeAndPosition();

#ifdef Q_OS_LINUX
        emit refreshMask();
#endif
}

//  Called when the user wants to maximize the palette
void WBDesktopPalette::maximizeMe()
{
    QList<QAction*> actions;
    clearLayout();

    actions << mActionUniboard;
    actions << WBApplication::mainWindow->actionPen;
    actions << WBApplication::mainWindow->actionEraser;
    actions << WBApplication::mainWindow->actionMarker;
    actions << WBApplication::mainWindow->actionSelector;
    actions << WBApplication::mainWindow->actionPointer;
    if (WBPlatformUtils::hasVirtualKeyboard())
        actions << WBApplication::mainWindow->actionVirtualKeyboard;

    actions << mActionCustomSelect;
    actions << mDisplaySelectAction;
    actions << mShowHideAction;

    setActions(actions);

    adjustSizeAndPosition();

    // Notify that the maximization has been done
    emit maximized();
#ifdef Q_OS_LINUX
        emit refreshMask();
#endif
}

void WBDesktopPalette::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);
    QIcon penIcon;
    QIcon markerIcon;
    QIcon eraserIcon;
    penIcon.addPixmap(QPixmap(":images/stylusPalette/penArrow.png"), QIcon::Normal, QIcon::Off);
    penIcon.addPixmap(QPixmap(":images/stylusPalette/penOnArrow.png"), QIcon::Normal, QIcon::On);
    WBApplication::mainWindow->actionPen->setIcon(penIcon);
    markerIcon.addPixmap(QPixmap(":images/stylusPalette/markerArrow.png"), QIcon::Normal, QIcon::Off);
    markerIcon.addPixmap(QPixmap(":images/stylusPalette/markerOnArrow.png"), QIcon::Normal, QIcon::On);
    WBApplication::mainWindow->actionMarker->setIcon(markerIcon);
    eraserIcon.addPixmap(QPixmap(":images/stylusPalette/eraserArrow.png"), QIcon::Normal, QIcon::Off);
    eraserIcon.addPixmap(QPixmap(":images/stylusPalette/eraserOnArrow.png"), QIcon::Normal, QIcon::On);
    WBApplication::mainWindow->actionEraser->setIcon(eraserIcon);

    adjustPosition();
}

void WBDesktopPalette::hideEvent(QHideEvent *event)
{
    Q_UNUSED(event);
    QIcon penIcon;
    QIcon markerIcon;
    QIcon eraserIcon;
    penIcon.addPixmap(QPixmap(":images/stylusPalette/pen.png"), QIcon::Normal, QIcon::Off);
    penIcon.addPixmap(QPixmap(":images/stylusPalette/penOn.png"), QIcon::Normal, QIcon::On);
    WBApplication::mainWindow->actionPen->setIcon(penIcon);
    markerIcon.addPixmap(QPixmap(":images/stylusPalette/marker.png"), QIcon::Normal, QIcon::Off);
    markerIcon.addPixmap(QPixmap(":images/stylusPalette/markerOn.png"), QIcon::Normal, QIcon::On);
    WBApplication::mainWindow->actionMarker->setIcon(markerIcon);
    eraserIcon.addPixmap(QPixmap(":images/stylusPalette/eraser.png"), QIcon::Normal, QIcon::Off);
    eraserIcon.addPixmap(QPixmap(":images/stylusPalette/eraserOn.png"), QIcon::Normal, QIcon::On);
    WBApplication::mainWindow->actionEraser->setIcon(eraserIcon);
}

QPoint WBDesktopPalette::buttonPos(QAction *action)
{
    QPoint p;

    WBActionPaletteButton* pB = mMapActionToButton[action];
    if(NULL != pB)
    {
        p = pB->pos();
    }

    return p;
}


int WBDesktopPalette::getParentRightOffset()
{
    return rightPalette->width();
}

void WBDesktopPalette::parentResized()
{
    QPoint p = pos();
    if (minimizedLocation() == eMinimizedLocation_Right)
    {
        p.setX(parentWidget()->width() - getParentRightOffset() -width());
    }

    moveInsideParent(p);
}
