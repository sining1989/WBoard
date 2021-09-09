#include "WBDisplayManager.h"

#include "frameworks/WBPlatformUtils.h"

#include "core/WBApplication.h"
#include "core/WBApplicationController.h"
#include "core/WBSettings.h"

#include "board/WBBoardView.h"
#include "board/WBBoardController.h"

#include "gui/WBBlackoutWidget.h"

#include "ui_blackoutWidget.h"

#include "core/memcheck.h"

#include "qdesktopwidget.h"

WBDisplayManager::WBDisplayManager(QObject *parent)
    : QObject(parent)
    , mControlScreenIndex(-1)
    , mDisplayScreenIndex(-1)
    , mControlWidget(0)
    , mDisplayWidget(0)
    , mDesktopWidget(0)
{
    mDesktop = qApp->desktop();

    mUseMultiScreen = WBSettings::settings()->appUseMultiscreen->get().toBool();

    initScreenIndexes();

    connect(mDesktop, &QDesktopWidget::resized, this, &WBDisplayManager::adjustScreens);
    connect(mDesktop, &QDesktopWidget::workAreaResized, this, &WBDisplayManager::adjustScreens);
}


void WBDisplayManager::initScreenIndexes()
{
    int screenCount = numScreens();

    mScreenIndexesRoles.clear();

    if (screenCount > 0)
    {
        mControlScreenIndex = mDesktop->primaryScreen();
        if (screenCount > 1 && WBSettings::settings()->swapControlAndDisplayScreens->get().toBool())
        {
            mControlScreenIndex = mControlScreenIndex^1;
        }

        mScreenIndexesRoles << Control;
    }
    else
    {
        mControlScreenIndex = -1;
    }

    if (screenCount > 1 && mUseMultiScreen)
    {
        mDisplayScreenIndex = mControlScreenIndex != 0 ? 0 : 1;
        mScreenIndexesRoles << Display;
    }
    else
    {
        mDisplayScreenIndex = -1;
    }

    mPreviousScreenIndexes.clear();

    if (screenCount > 2)
    {
        for(int i = 2; i < screenCount; i++)
        {
            if(mControlScreenIndex == i)
                mPreviousScreenIndexes.append(1);
            else
                mPreviousScreenIndexes.append(i);
        }
    }
}

void WBDisplayManager::swapDisplayScreens(bool swap)
{
    initScreenIndexes();

    if (swap)
    {
        // As it s a really specific ask and we don't have much time to handle it correctly
        // this code handles only the swap between the main display screen and the first previous one
        int displayScreenIndex = mDisplayScreenIndex;
        mDisplayScreenIndex = mPreviousScreenIndexes.at(0);
        mPreviousScreenIndexes.clear();
        mPreviousScreenIndexes.append(displayScreenIndex);
    }

    positionScreens();

    emit screenLayoutChanged();
    emit adjustDisplayViewsRequired();
}


WBDisplayManager::~WBDisplayManager()
{
    // NOOP
}


int WBDisplayManager::numScreens()
{
    int screenCount = mDesktop->screenCount();
    // Some window managers report two screens when the two monitors are in "cloned" mode; this hack ensures
    // that we consider this as just one screen. On most desktops, at least one of the following conditions is
    // a good indicator of the displays being in cloned or extended mode.
#ifdef Q_OS_LINUX
    if (screenCount > 1
        && (mDesktop->screenNumber(mDesktop->screen(0)) == mDesktop->screenNumber(mDesktop->screen(1))
            || mDesktop->screenGeometry(0) == mDesktop->screenGeometry(1)))
        return 1;
#endif
    return screenCount;
}


int WBDisplayManager::numPreviousViews()
{
    return mPreviousScreenIndexes.size();
}


void WBDisplayManager::setControlWidget(QWidget* pControlWidget)
{
    if(hasControl() && pControlWidget && (pControlWidget != mControlWidget))
        mControlWidget = pControlWidget;
}

void WBDisplayManager::setDesktopWidget(QWidget* pControlWidget )
{
    if(pControlWidget && (pControlWidget != mControlWidget))
        mDesktopWidget = pControlWidget;
}

void WBDisplayManager::setDisplayWidget(QWidget* pDisplayWidget)
{
    if(pDisplayWidget && (pDisplayWidget != mDisplayWidget))
    {
        if (mDisplayWidget)
        {
            mDisplayWidget->hide();
            pDisplayWidget->setGeometry(mDisplayWidget->geometry());
            pDisplayWidget->setWindowFlags(mDisplayWidget->windowFlags());
        }
        mDisplayWidget = pDisplayWidget;
        mDisplayWidget->setGeometry(mDesktop->screenGeometry(mDisplayScreenIndex));
        if (WBSettings::settings()->appUseMultiscreen->get().toBool())
            WBPlatformUtils::showFullScreen(mDisplayWidget);
    }
}


void WBDisplayManager::setPreviousDisplaysWidgets(QList<WBBoardView*> pPreviousViews)
{
    mPreviousDisplayWidgets = pPreviousViews;
}


QRect WBDisplayManager::controlGeometry()
{
    return mDesktop->screenGeometry(mControlScreenIndex);
}

QRect WBDisplayManager::displayGeometry()
{
    return mDesktop->screenGeometry(mDisplayScreenIndex);
}

void WBDisplayManager::reinitScreens(bool swap)
{
    Q_UNUSED(swap);
    adjustScreens(-1);
}

void WBDisplayManager::adjustScreens(int screen)
{
    Q_UNUSED(screen);

    initScreenIndexes();

    positionScreens();

    emit screenLayoutChanged();
}


void WBDisplayManager::positionScreens()
{

    if(mDesktopWidget && mControlScreenIndex > -1)
    {
        mDesktopWidget->hide();
        mDesktopWidget->setGeometry(mDesktop->screenGeometry(mControlScreenIndex));
    }
    if (mControlWidget && mControlScreenIndex > -1)
    {
        mControlWidget->hide();
        mControlWidget->setGeometry(mDesktop->screenGeometry(mControlScreenIndex));
        WBPlatformUtils::showFullScreen(mControlWidget);
    }

    if (mDisplayWidget && mDisplayScreenIndex > -1)
    {
        mDisplayWidget->hide();
        mDisplayWidget->setGeometry(mDesktop->screenGeometry(mDisplayScreenIndex));
        WBPlatformUtils::showFullScreen(mDisplayWidget);
    }
    else if(mDisplayWidget)
    {
        mDisplayWidget->hide();
    }

    for (int wi = mPreviousScreenIndexes.size(); wi < mPreviousDisplayWidgets.size(); wi++)
    {
        mPreviousDisplayWidgets.at(wi)->hide();
    }

    for (int psi = 0; psi < mPreviousScreenIndexes.size(); psi++)
    {
        if (mPreviousDisplayWidgets.size() > psi)
        {
            QWidget* previous = mPreviousDisplayWidgets.at(psi);
            previous->setGeometry(mDesktop->screenGeometry(mPreviousScreenIndexes.at(psi)));
            WBPlatformUtils::showFullScreen(previous);
        }
    }

    if (mControlWidget && mControlScreenIndex > -1)
        mControlWidget->activateWindow();

}


void WBDisplayManager::blackout()
{
    QList<int> screenIndexes;

    if (mControlScreenIndex > -1)
        screenIndexes << mControlScreenIndex;

    if (mDisplayScreenIndex > -1)
        screenIndexes << mDisplayScreenIndex;

    screenIndexes << mPreviousScreenIndexes;

    for (int i = 0; i < screenIndexes.size(); i++)
    {
        int screenIndex = screenIndexes.at(i);

        WBBlackoutWidget *blackoutWidget = new WBBlackoutWidget(); //deleted in WBDisplayManager::unBlackout
        Ui::BlackoutWidget *blackoutUi = new Ui::BlackoutWidget();
        blackoutUi->setupUi(blackoutWidget);

        connect(blackoutUi->iconButton, SIGNAL(pressed()), blackoutWidget, SLOT(doActivity()));
        connect(blackoutWidget, SIGNAL(activity()), this, SLOT(unBlackout()));

        // display Uniboard logo on main screen
        blackoutUi->iconButton->setVisible(screenIndex == mControlScreenIndex);
        blackoutUi->labelClickToReturn->setVisible(screenIndex == mControlScreenIndex);

        blackoutWidget->setGeometry(mDesktop->screenGeometry(screenIndex));

        mBlackoutWidgets << blackoutWidget;
    }

    WBPlatformUtils::fadeDisplayOut();

    foreach(WBBlackoutWidget *blackoutWidget, mBlackoutWidgets)
    {
        WBPlatformUtils::showFullScreen(blackoutWidget);
    }
}

void WBDisplayManager::unBlackout()
{
    while (!mBlackoutWidgets.isEmpty())
    {
        // the widget is also destroyed thanks to its Qt::WA_DeleteOnClose attribute
        mBlackoutWidgets.takeFirst()->close();
    }

    WBPlatformUtils::fadeDisplayIn();

    WBApplication::boardController->freezeW3CWidgets(false);

}


void WBDisplayManager::setRoleToScreen(DisplayRole role, int screenIndex)
{
    Q_UNUSED(role);
    Q_UNUSED(screenIndex);
}


void WBDisplayManager::setUseMultiScreen(bool pUse)
{
    mUseMultiScreen = pUse;
}

