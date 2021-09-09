#include "core/WBApplication.h"
#include "board/WBBoardController.h"

#include "WBRightPalette.h"

#include "core/memcheck.h"

WBRightPalette::WBRightPalette(QWidget *parent, const char *name):
    WBDockPalette(eWBDockPaletteType_RIGHT, parent)
{
    setObjectName(name);
    setOrientation(eWBDockOrientation_Right);
    mCollapseWidth = 120;
    bool isCollapsed = false;
    if(mCurrentMode == eWBDockPaletteWidget_BOARD){
        mLastWidth = WBSettings::settings()->rightLibPaletteBoardModeWidth->get().toInt();
        isCollapsed = WBSettings::settings()->rightLibPaletteBoardModeIsCollapsed->get().toBool();
    }
    else{
        mLastWidth = WBSettings::settings()->rightLibPaletteDesktopModeWidth->get().toInt();
        isCollapsed = WBSettings::settings()->rightLibPaletteDesktopModeIsCollapsed->get().toBool();
    }
    if(isCollapsed)
        resize(0,parentWidget()->height());
    else
        resize(mLastWidth, parentWidget()->height());
}

WBRightPalette::~WBRightPalette()
{
}

void WBRightPalette::mouseMoveEvent(QMouseEvent *event)
{
    if(mCanResize)
    {
        WBDockPalette::mouseMoveEvent(event);
    }
}

void WBRightPalette::resizeEvent(QResizeEvent *event)
{
    int newWidth = width();
    if(mCurrentMode == eWBDockPaletteWidget_BOARD){
        if(newWidth > mCollapseWidth)
            WBSettings::settings()->rightLibPaletteBoardModeWidth->set(newWidth);
        WBSettings::settings()->rightLibPaletteBoardModeIsCollapsed->set(newWidth == 0);
    }
    else{
        if(newWidth > mCollapseWidth)
            WBSettings::settings()->rightLibPaletteDesktopModeWidth->set(newWidth);
        WBSettings::settings()->rightLibPaletteDesktopModeIsCollapsed->set(newWidth == 0);
    }
    WBDockPalette::resizeEvent(event);
    emit resized();
}

void WBRightPalette::updateMaxWidth()
{
    setMaximumWidth((int)(parentWidget()->width() * 0.45));
    setMaximumHeight(parentWidget()->height());
    setMinimumHeight(parentWidget()->height());
}

bool WBRightPalette::switchMode(eWBDockPaletteWidgetMode mode)
{
    int newModeWidth;
    if(mode == eWBDockPaletteWidget_BOARD){
        mLastWidth = WBSettings::settings()->rightLibPaletteBoardModeWidth->get().toInt();
        newModeWidth = mLastWidth;
        if(WBSettings::settings()->rightLibPaletteBoardModeIsCollapsed->get().toBool())
            newModeWidth = 0;
    }
    else{
        mLastWidth = WBSettings::settings()->rightLibPaletteDesktopModeWidth->get().toInt();
        newModeWidth = mLastWidth;
        if(WBSettings::settings()->rightLibPaletteDesktopModeIsCollapsed->get().toBool())
            newModeWidth = 0;
    }
    resize(newModeWidth,height());
    return WBDockPalette::switchMode(mode);
}
