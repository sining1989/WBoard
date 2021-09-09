#include "WBLeftPalette.h"
#include "core/WBSettings.h"

#include "core/memcheck.h"


WBLeftPalette::WBLeftPalette(QWidget *parent, const char *name):
    WBDockPalette(eWBDockPaletteType_LEFT, parent)
{
    setObjectName(name);
    setOrientation(eWBDockOrientation_Left);
    mCollapseWidth = 120;

    bool isCollapsed = false;
    if(mCurrentMode == eWBDockPaletteWidget_BOARD){
        mLastWidth = WBSettings::settings()->leftLibPaletteBoardModeWidth->get().toInt();
        isCollapsed = WBSettings::settings()->leftLibPaletteBoardModeIsCollapsed->get().toBool();
    }
    else{
        mLastWidth = WBSettings::settings()->leftLibPaletteDesktopModeWidth->get().toInt();
        isCollapsed = WBSettings::settings()->leftLibPaletteDesktopModeIsCollapsed->get().toBool();
    }

    if(isCollapsed)
        resize(0,parentWidget()->height());
    else
        resize(mLastWidth, parentWidget()->height());
}

/**
 * \brief The destructor
 */
WBLeftPalette::~WBLeftPalette()
{

}


void WBLeftPalette::onDocumentSet(WBDocumentProxy* documentProxy)
{
    Q_UNUSED(documentProxy)
    // the tab zero is forced
    mLastOpenedTabForMode.insert(eWBDockPaletteWidget_BOARD, 0);
}

/**
 * \brief Update the maximum width
 */
void WBLeftPalette::updateMaxWidth()
{
    setMaximumWidth((int)(parentWidget()->width() * 0.45));
}

/**
 * \brief Handle the resize event
 * @param event as the resize event
 */
void WBLeftPalette::resizeEvent(QResizeEvent *event)
{
    int newWidth = width();
    if(mCurrentMode == eWBDockPaletteWidget_BOARD){
        if(newWidth > mCollapseWidth)
            WBSettings::settings()->leftLibPaletteBoardModeWidth->set(newWidth);
        WBSettings::settings()->leftLibPaletteBoardModeIsCollapsed->set(newWidth == 0);
    }
    else{
        if(newWidth > mCollapseWidth)
            WBSettings::settings()->leftLibPaletteDesktopModeWidth->set(newWidth);
        WBSettings::settings()->leftLibPaletteDesktopModeIsCollapsed->set(newWidth == 0);
    }
    WBDockPalette::resizeEvent(event);
}


bool WBLeftPalette::switchMode(eWBDockPaletteWidgetMode mode)
{
    int newModeWidth;
    if(mode == eWBDockPaletteWidget_BOARD){
        mLastWidth = WBSettings::settings()->leftLibPaletteBoardModeWidth->get().toInt();
        newModeWidth = mLastWidth;
        if(WBSettings::settings()->leftLibPaletteBoardModeIsCollapsed->get().toBool())
            newModeWidth = 0;
    }
    else{
        mLastWidth = WBSettings::settings()->leftLibPaletteDesktopModeWidth->get().toInt();
        newModeWidth = mLastWidth;
        if(WBSettings::settings()->leftLibPaletteDesktopModeIsCollapsed->get().toBool())
            newModeWidth = 0;
    }
    resize(newModeWidth,height());
    return WBDockPalette::switchMode(mode);
}
