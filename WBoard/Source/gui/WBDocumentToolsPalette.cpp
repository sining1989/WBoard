#include "WBDocumentToolsPalette.h"

#include <QtWidgets>

#include "frameworks/WBPlatformUtils.h"

#include "core/WBApplication.h"
#include "gui/WBMainWindow.h"


#include "core/memcheck.h"

WBDocumentToolsPalette::WBDocumentToolsPalette(QWidget *parent)
    : WBActionPalette(Qt::TopRightCorner, parent)
{
    QList<QAction*> actions;

    if (WBPlatformUtils::hasVirtualKeyboard())
        actions << WBApplication::mainWindow->actionVirtualKeyboard;

    setActions(actions);
    setButtonIconSize(QSize(42, 42));

    adjustSizeAndPosition();
}


WBDocumentToolsPalette::~WBDocumentToolsPalette()
{
    // NOOP
}

