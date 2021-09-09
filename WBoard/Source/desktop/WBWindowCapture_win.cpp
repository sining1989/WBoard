#include "WBWindowCapture.h"

#include "WBWindowCaptureDelegate_win.h"
#include "WBDesktopAnnotationController.h"
#include "WBDesktopPalette.h"

#include "board/WBBoardView.h"

#include "core/memcheck.h"

WBWindowCapture::WBWindowCapture(WBDesktopAnnotationController *parent)
        : QObject(parent)
        , mParent(parent)
{
    // NOOP
}


WBWindowCapture::~WBWindowCapture()
{
    // NOOP
}


const QPixmap WBWindowCapture::getCapturedWindow()
{
    return mWindowPixmap;
}


int WBWindowCapture::execute()
{
    mParent->desktopPalette()->grabMouse();
    mParent->desktopPalette()->grabKeyboard();

    WBWindowCaptureDelegate windowCaptureEventHandler;
    int result = windowCaptureEventHandler.execute();

    mParent->desktopPalette()->releaseMouse();
    mParent->desktopPalette()->releaseKeyboard();

    mWindowPixmap = windowCaptureEventHandler.getCapturedWindow();

    return result;
}
