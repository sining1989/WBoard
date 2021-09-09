#include "WBGraphicsGroupContainerItemDelegate.h"

#include <QtWidgets>

#include "WBGraphicsScene.h"
#include "gui/WBResources.h"

#include "domain/WBGraphicsDelegateFrame.h"
#include "domain/WBGraphicsGroupContainerItem.h"

#include "board/WBBoardController.h"

#include "core/memcheck.h"

WBGraphicsGroupContainerItemDelegate::WBGraphicsGroupContainerItemDelegate(QGraphicsItem *pDelegated, QObject *parent) :
    WBGraphicsItemDelegate(pDelegated, parent, GF_COMMON | GF_RESPECT_RATIO), mDestroyGroupButton(0)
{
}

WBGraphicsGroupContainerItem *WBGraphicsGroupContainerItemDelegate::delegated()
{
    return dynamic_cast<WBGraphicsGroupContainerItem*>(mDelegated);
}

void WBGraphicsGroupContainerItemDelegate::decorateMenu(QMenu *menu)
{
    mLockAction = menu->addAction(tr("Locked"), this, SLOT(lock(bool)));
    QIcon lockIcon;
    lockIcon.addPixmap(QPixmap(":/images/locked.svg"), QIcon::Normal, QIcon::On);
    lockIcon.addPixmap(QPixmap(":/images/unlocked.svg"), QIcon::Normal, QIcon::Off);
    mLockAction->setIcon(lockIcon);
    mLockAction->setCheckable(true);

    mShowOnDisplayAction = mMenu->addAction(tr("Visible on Extended Screen"), this, SLOT(showHide(bool)));
    mShowOnDisplayAction->setCheckable(true);

    QIcon showIcon;
    showIcon.addPixmap(QPixmap(":/images/eyeOpened.svg"), QIcon::Normal, QIcon::On);
    showIcon.addPixmap(QPixmap(":/images/eyeClosed.svg"), QIcon::Normal, QIcon::Off);
    mShowOnDisplayAction->setIcon(showIcon);
}

void WBGraphicsGroupContainerItemDelegate::buildButtons()
{
    if (!mDestroyGroupButton) {
        mDestroyGroupButton = new DelegateButton(":/images/ungroupItems.svg", mDelegated, mFrame, Qt::TopLeftSection);
        mDestroyGroupButton->setShowProgressIndicator(false);
        connect(mDestroyGroupButton, SIGNAL(clicked()), this, SLOT(destroyGroup()));
        mButtons << mDestroyGroupButton;
    }

    WBGraphicsItemDelegate::buildButtons();
}

void WBGraphicsGroupContainerItemDelegate::freeButtons()
{
    WBGraphicsItemDelegate::freeButtons();
    mDestroyGroupButton = 0;
    mButtons.clear();
}

bool WBGraphicsGroupContainerItemDelegate::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event)
    delegated()->deselectCurrentItem();
    return false;
}

bool WBGraphicsGroupContainerItemDelegate::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event)

    return false;
}

bool WBGraphicsGroupContainerItemDelegate::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event)

    return false;
}

void WBGraphicsGroupContainerItemDelegate::destroyGroup()
{
    qDebug() << "Destroying group";
    delegated()->destroy();
}
