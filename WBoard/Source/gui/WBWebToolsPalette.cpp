#include <QtWidgets>

#include "frameworks/WBPlatformUtils.h"

#include "core/WBApplication.h"
#include "core/WBSettings.h"
#include "core/WBApplicationController.h"
#include "core/WBDisplayManager.h"

#include "gui/WBMainWindow.h"

#include "WBWebToolsPalette.h"
#include "WBResources.h"
#include "WBIconButton.h"

#include "core/memcheck.h"

WBWebToolsPalette::WBWebToolsPalette(QWidget *parent)
    : WBActionPalette(Qt::TopRightCorner, parent)
{
    QList<QAction*> actions;

    actions << WBApplication::mainWindow->actionWebCustomCapture;
    //actions << WBApplication::mainWindow->actionWebWindowCapture;//zhusizhi
    //actions << WBApplication::mainWindow->actionWebOEmbed;

    actions << WBApplication::mainWindow->actionWebShowHideOnDisplay;

    if (WBPlatformUtils::hasVirtualKeyboard())
        actions << WBApplication::mainWindow->actionVirtualKeyboard;

    setActions(actions);
    setButtonIconSize(QSize(42, 42));
    adjustSizeAndPosition();
}


WBWebToolsPalette::~WBWebToolsPalette()
{
    // NOOP
}



