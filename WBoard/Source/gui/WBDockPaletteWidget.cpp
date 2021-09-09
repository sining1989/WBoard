#include "WBDockPaletteWidget.h"

#include "core/memcheck.h"

WBDockPaletteWidget::WBDockPaletteWidget(QWidget *parent, const char *name):QWidget(parent)
{
    setObjectName(name);
}

WBDockPaletteWidget::~WBDockPaletteWidget()
{

}

QPixmap WBDockPaletteWidget::iconToRight()
{
    return mIconToRight;
}

QPixmap WBDockPaletteWidget::iconToLeft()
{
    return mIconToLeft;
}

QString WBDockPaletteWidget::name()
{
    return mName;
}

/**
  * When a widget registers a mode it means that it would be displayed on that mode
  */
void WBDockPaletteWidget::registerMode(eWBDockPaletteWidgetMode mode)
{
    if(!mRegisteredModes.contains(mode))
        mRegisteredModes.append(mode);
}

void WBDockPaletteWidget::slot_changeMode(eWBDockPaletteWidgetMode newMode)
{
    this->setVisible(this->visibleInMode( newMode ));
}


