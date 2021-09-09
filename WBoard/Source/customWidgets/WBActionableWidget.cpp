#include <QPainter>
#include <QDebug>

#include "WBActionableWidget.h"

#include "core/memcheck.h"

WBActionableWidget::WBActionableWidget(QWidget *parent, const char *name):QWidget(parent)
  , mShowActions(false)
{
    setObjectName(name);
    mActions.clear();
    mCloseButtons.setIcon(QIcon(QPixmap(":images/close.svg")));
    mCloseButtons.setGeometry(0, 0, 2*ACTIONSIZE, ACTIONSIZE);
    mCloseButtons.setVisible(false);
    connect(&mCloseButtons, SIGNAL(clicked()), this, SLOT(onCloseClicked()));
}

WBActionableWidget::~WBActionableWidget()
{

}

void WBActionableWidget::addAction(eAction act)
{
    if(!mActions.contains(act)){
        mActions << act;
    }
}

void WBActionableWidget::removeAction(eAction act)
{
    if(mActions.contains(act)){
        mActions.remove(mActions.indexOf(act));
    }
}

void WBActionableWidget::removeAllActions()
{
    mActions.clear();
}

void WBActionableWidget::setActionsVisible(bool bVisible)
{
    if(!mActions.empty() && mActions.contains(eAction_Close)){
        mCloseButtons.setVisible(bVisible);
    }
}

void WBActionableWidget::onCloseClicked()
{
    emit close(this);
}

void WBActionableWidget::setActionsParent(QWidget *parent)
{
    if(mActions.contains(eAction_Close)){
        mCloseButtons.setParent(parent);
    }
}

void WBActionableWidget::unsetActionsParent()
{
    if(mActions.contains(eAction_Close)){
        mCloseButtons.setParent(this);
    }
}
