#include <QtWidgets>
#include <QtSvg>

#include "WBGraphicsWidgetItemDelegate.h"
#include "WBGraphicsScene.h"

#include "core/WBApplication.h"
#include "gui/WBMainWindow.h"
#include "board/WBBoardController.h"
#include "board/WBBoardView.h"

#include "domain/WBGraphicsWidgetItem.h"
#include "domain/WBGraphicsDelegateFrame.h"

#include "core/memcheck.h"

WBGraphicsWidgetItemDelegate::WBGraphicsWidgetItemDelegate(WBGraphicsWidgetItem* pDelegated, int widgetType)
    : WBGraphicsItemDelegate(pDelegated, 0, GF_COMMON
                             | GF_RESPECT_RATIO)
    , freezeAction(0)
    , setAsToolAction(0)
{
    mWidgetType = widgetType;
}


WBGraphicsWidgetItemDelegate::~WBGraphicsWidgetItemDelegate()
{
    // NOOP
}


void WBGraphicsWidgetItemDelegate::pin()
{
    WBApplication::boardController->moveGraphicsWidgetToControlView(delegated());
}


void WBGraphicsWidgetItemDelegate::updateMenuActionState()
{
    WBGraphicsItemDelegate::updateMenuActionState();

    if (freezeAction)
        freezeAction->setChecked(delegated()->isFrozen());
}

void WBGraphicsWidgetItemDelegate::decorateMenu(QMenu* menu)
{
    WBGraphicsItemDelegate::decorateMenu(menu);

    freezeAction = menu->addAction(tr("Frozen"), this, SLOT(freeze(bool)));

    QIcon freezeIcon;
    freezeIcon.addPixmap(QPixmap(":/images/frozen.svg"), QIcon::Normal, QIcon::On);
    freezeIcon.addPixmap(QPixmap(":/images/unfrozen.svg"), QIcon::Normal, QIcon::Off);
    freezeAction->setIcon(freezeIcon);

    freezeAction->setCheckable(true);

    if (delegated()->canBeTool())
    {
        setAsToolAction = mMenu->addAction(tr("Transform as Tool "), this, SLOT(pin()));
        QIcon pinIcon;
        pinIcon.addPixmap(QPixmap(":/images/unpin.svg"), QIcon::Normal, QIcon::On);
        pinIcon.addPixmap(QPixmap(":/images/pin.svg"), QIcon::Normal, QIcon::Off);
        setAsToolAction->setIcon(pinIcon);
    }
}


void WBGraphicsWidgetItemDelegate::freeze(bool frozen)
{
    if(frozen)
    {
       delegated()->freeze();
    }
    else
    {
       delegated()->unFreeze();
    }
}


WBGraphicsWidgetItem* WBGraphicsWidgetItemDelegate::delegated()
{
    return static_cast<WBGraphicsWidgetItem*>(mDelegated);
}


void WBGraphicsWidgetItemDelegate::remove(bool canundo)
{
    delegated()->removeScript();
    WBGraphicsItemDelegate::remove(canundo);
}
