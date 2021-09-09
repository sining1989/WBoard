#include "WBDrawingController.h"

#include "core/WBSettings.h"
#include "core/WBApplication.h"

#include "domain/WBGraphicsScene.h"
#include "board/WBBoardController.h"

#include "gui/WBMainWindow.h"
#include "gui/WBActionPalette.h"
#include "core/memcheck.h"

WBDrawingController* WBDrawingController::sDrawingController = 0;

WBDrawingController* WBDrawingController::drawingController()
{
    if(!sDrawingController)
        sDrawingController = new WBDrawingController();

    return sDrawingController;
}

void WBDrawingController::destroy()
{
    if(sDrawingController)
        delete sDrawingController;
    sDrawingController = NULL;
}

WBDrawingController::WBDrawingController(QObject * parent)
    : QObject(parent)
    , mActiveRuler(NULL)
    , mStylusTool((WBStylusTool::Enum)-1)
    , mLatestDrawingTool((WBStylusTool::Enum)-1)
    , mIsDesktopMode(false)
{
    connect(WBSettings::settings(), SIGNAL(colorContextChanged()), this, SIGNAL(colorPaletteChanged()));

    connect(WBApplication::mainWindow->actionPen, SIGNAL(triggered(bool)), this, SLOT(penToolSelected(bool)));
    connect(WBApplication::mainWindow->actionEraser, SIGNAL(triggered(bool)), this, SLOT(eraserToolSelected(bool)));
    connect(WBApplication::mainWindow->actionMarker, SIGNAL(triggered(bool)), this, SLOT(markerToolSelected(bool)));
    connect(WBApplication::mainWindow->actionSelector, SIGNAL(triggered(bool)), this, SLOT(selectorToolSelected(bool)));
    connect(WBApplication::mainWindow->actionPlay, SIGNAL(triggered(bool)), this, SLOT(playToolSelected(bool)));
    connect(WBApplication::mainWindow->actionHand, SIGNAL(triggered(bool)), this, SLOT(handToolSelected(bool)));
    connect(WBApplication::mainWindow->actionZoomIn, SIGNAL(triggered(bool)), this, SLOT(zoomInToolSelected(bool)));
    connect(WBApplication::mainWindow->actionZoomOut, SIGNAL(triggered(bool)), this, SLOT(zoomOutToolSelected(bool)));
    connect(WBApplication::mainWindow->actionPointer, SIGNAL(triggered(bool)), this, SLOT(pointerToolSelected(bool)));
    connect(WBApplication::mainWindow->actionLine, SIGNAL(triggered(bool)), this, SLOT(lineToolSelected(bool)));
    connect(WBApplication::mainWindow->actionText, SIGNAL(triggered(bool)), this, SLOT(textToolSelected(bool)));
    connect(WBApplication::mainWindow->actionCapture, SIGNAL(triggered(bool)), this, SLOT(captureToolSelected(bool)));

}

WBDrawingController::~WBDrawingController()
{
    // NOOP
}

int WBDrawingController::stylusTool()
{
    return mStylusTool;
}

int WBDrawingController::latestDrawingTool()
{
    return mLatestDrawingTool;
}

void WBDrawingController::setStylusTool(int tool)
{
    if (tool != mStylusTool)
    {
        WBApplication::boardController->activeScene()->deselectAllItems();
        if (mStylusTool == WBStylusTool::Pen || mStylusTool == WBStylusTool::Marker
                || mStylusTool == WBStylusTool::Line)
        {
            mLatestDrawingTool = mStylusTool;
        }

        if (tool == WBStylusTool::Pen || tool == WBStylusTool::Line)
        {
             emit lineWidthIndexChanged(WBSettings::settings()->penWidthIndex());
             emit colorIndexChanged(WBSettings::settings()->penColorIndex());
        }
        else if (tool == WBStylusTool::Marker)
        {
            emit lineWidthIndexChanged(WBSettings::settings()->markerWidthIndex());
            emit colorIndexChanged(WBSettings::settings()->markerColorIndex());
        }

        mStylusTool = (WBStylusTool::Enum)tool;

        if (mStylusTool == WBStylusTool::Pen)
            WBApplication::mainWindow->actionPen->setChecked(true);
        else if (mStylusTool == WBStylusTool::Eraser)
            WBApplication::mainWindow->actionEraser->setChecked(true);
        else if (mStylusTool == WBStylusTool::Marker)
            WBApplication::mainWindow->actionMarker->setChecked(true);
        else if (mStylusTool == WBStylusTool::Selector)
            WBApplication::mainWindow->actionSelector->setChecked(true);
        else if (mStylusTool == WBStylusTool::Play)
            WBApplication::mainWindow->actionPlay->setChecked(true);
        else if (mStylusTool == WBStylusTool::Hand)
            WBApplication::mainWindow->actionHand->setChecked(true);
        else if (mStylusTool == WBStylusTool::ZoomIn)
            WBApplication::mainWindow->actionZoomIn->setChecked(true);
        else if (mStylusTool == WBStylusTool::ZoomOut)
            WBApplication::mainWindow->actionZoomOut->setChecked(true);
        else if (mStylusTool == WBStylusTool::Pointer)
            WBApplication::mainWindow->actionPointer->setChecked(true);
        else if (mStylusTool == WBStylusTool::Line)
            WBApplication::mainWindow->actionLine->setChecked(true);
        else if (mStylusTool == WBStylusTool::Text)
            WBApplication::mainWindow->actionText->setChecked(true);
        else if (mStylusTool == WBStylusTool::Capture)
            WBApplication::mainWindow->actionCapture->setChecked(true);

        emit stylusToolChanged(tool);
        if (mStylusTool != WBStylusTool::Selector)
            emit colorPaletteChanged();
    }
}

bool WBDrawingController::isDrawingTool()
{
    return (stylusTool() == WBStylusTool::Pen)
            || (stylusTool() == WBStylusTool::Marker)
            || (stylusTool() == WBStylusTool::Line);
}

int WBDrawingController::currentToolWidthIndex()
{
    if (stylusTool() == WBStylusTool::Pen || stylusTool() == WBStylusTool::Line)
        return WBSettings::settings()->penWidthIndex();
    else if (stylusTool() == WBStylusTool::Marker)
        return WBSettings::settings()->markerWidthIndex();
    else
        return -1;
}

qreal WBDrawingController::currentToolWidth()
{
    if (stylusTool() == WBStylusTool::Pen || stylusTool() == WBStylusTool::Line)
        return WBSettings::settings()->currentPenWidth();
    else if (stylusTool() == WBStylusTool::Marker)
        return WBSettings::settings()->currentMarkerWidth();
    else
        //failsafe
        return WBSettings::settings()->currentPenWidth();
}

void WBDrawingController::setLineWidthIndex(int index)
{
    if (stylusTool() == WBStylusTool::Marker)
    {
        WBSettings::settings()->setMarkerWidthIndex(index);
    }
    else
    {
        WBSettings::settings()->setPenWidthIndex(index);

        if(stylusTool() != WBStylusTool::Line
            && stylusTool() != WBStylusTool::Selector)
        {
            setStylusTool(WBStylusTool::Pen);
        }
    }

    emit lineWidthIndexChanged(index);
}


int WBDrawingController::currentToolColorIndex()
{
    if (stylusTool() == WBStylusTool::Pen || stylusTool() == WBStylusTool::Line)
    {
        return WBSettings::settings()->penColorIndex();
    }
    else if (stylusTool() == WBStylusTool::Marker)
    {
        return WBSettings::settings()->markerColorIndex();
    }
    else
    {
        return -1;
    }
}

QColor WBDrawingController::currentToolColor()
{
    return toolColor(WBSettings::settings()->isDarkBackground());
}

QColor WBDrawingController::toolColor(bool onDarkBackground)
{
    if (stylusTool() == WBStylusTool::Pen || stylusTool() == WBStylusTool::Line)
    {
        return WBSettings::settings()->penColor(onDarkBackground);
    }
    else if (stylusTool() == WBStylusTool::Marker)
    {
        return WBSettings::settings()->markerColor(onDarkBackground);
    }
    else
    {
        //failsafe
        if (onDarkBackground)
        {
            return Qt::white;
        }
        else
        {
            return Qt::black;
        }
    }
}

void WBDrawingController::setColorIndex(int index)
{
    Q_ASSERT(index >= 0 && index < WBSettings::settings()->colorPaletteSize);

    if (stylusTool() == WBStylusTool::Marker)
    {
        WBSettings::settings()->setMarkerColorIndex(index);
    }
    else
    {
        WBSettings::settings()->setPenColorIndex(index);
    }

    emit colorIndexChanged(index);
}

void WBDrawingController::setEraserWidthIndex(int index)
{
    setStylusTool(WBStylusTool::Eraser);
    WBSettings::settings()->setEraserWidthIndex(index);
}

void WBDrawingController::setPenColor(bool onDarkBackground, const QColor& color, int pIndex)
{
    if (onDarkBackground)
    {
        WBSettings::settings()->boardPenDarkBackgroundSelectedColors->setColor(pIndex, color);
    }
    else
    {
        WBSettings::settings()->boardPenLightBackgroundSelectedColors->setColor(pIndex, color);
    }

    emit colorPaletteChanged();
}

void WBDrawingController::setMarkerColor(bool onDarkBackground, const QColor& color, int pIndex)
{
    if (onDarkBackground)
    {
        WBSettings::settings()->boardMarkerDarkBackgroundSelectedColors->setColor(pIndex, color);
    }
    else
    {
        WBSettings::settings()->boardMarkerLightBackgroundSelectedColors->setColor(pIndex, color);
    }

    emit colorPaletteChanged();
}

void WBDrawingController::setMarkerAlpha(qreal alpha)
{
    WBSettings::settings()->boardMarkerLightBackgroundColors->setAlpha(alpha);
    WBSettings::settings()->boardMarkerLightBackgroundSelectedColors->setAlpha(alpha);

    WBSettings::settings()->boardMarkerDarkBackgroundColors->setAlpha(alpha);
    WBSettings::settings()->boardMarkerDarkBackgroundSelectedColors->setAlpha(alpha);

    WBSettings::settings()->boardMarkerAlpha->set(alpha);

    emit colorPaletteChanged();
}


void WBDrawingController::penToolSelected(bool checked)
{
    if (checked)
        setStylusTool(WBStylusTool::Pen);
}

void WBDrawingController::eraserToolSelected(bool checked)
{
    if (checked)
        setStylusTool(WBStylusTool::Eraser);
}

void WBDrawingController::markerToolSelected(bool checked)
{
    if (checked)
        setStylusTool(WBStylusTool::Marker);

}

void WBDrawingController::selectorToolSelected(bool checked)
{
    if (checked)
        setStylusTool(WBStylusTool::Selector);
}

void WBDrawingController::playToolSelected(bool checked)
{
    if (checked)
        setStylusTool(WBStylusTool::Play);
}

void WBDrawingController::handToolSelected(bool checked)
{
    if (checked)
        setStylusTool(WBStylusTool::Hand);
}


void WBDrawingController::zoomInToolSelected(bool checked)
{
    if (checked)
        setStylusTool(WBStylusTool::ZoomIn);
}

void WBDrawingController::zoomOutToolSelected(bool checked)
{
    if (checked)
        setStylusTool(WBStylusTool::ZoomOut);
}

void WBDrawingController::pointerToolSelected(bool checked)
{
    if (checked)
        setStylusTool(WBStylusTool::Pointer);
}


void WBDrawingController::lineToolSelected(bool checked)
{
    if (checked)
        setStylusTool(WBStylusTool::Line);
}

void WBDrawingController::textToolSelected(bool checked)
{
    if (checked)
        setStylusTool(WBStylusTool::Text);
}

void WBDrawingController::captureToolSelected(bool checked)
{
    if (checked)
        setStylusTool(WBStylusTool::Capture);
}
