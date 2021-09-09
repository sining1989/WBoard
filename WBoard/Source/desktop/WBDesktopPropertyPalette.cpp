#include "WBDesktopPropertyPalette.h"

#include "core/WBApplication.h"
#include "board/WBBoardController.h"
#include "board/WBDrawingController.h"
#include "gui/WBMainWindow.h"
#include "gui/WBToolbarButtonGroup.h"
#include "gui/WBRightPalette.h"

#include "core/memcheck.h"

WBDesktopPropertyPalette::WBDesktopPropertyPalette(QWidget *parent, WBRightPalette* _rightPalette)
    :WBPropertyPalette(Qt::Horizontal, parent)
    ,rightPalette(_rightPalette)
{}

int WBDesktopPropertyPalette::getParentRightOffset()
{
    return rightPalette->width();
}


WBDesktopPenPalette::WBDesktopPenPalette(QWidget *parent, WBRightPalette* rightPalette)
    : WBDesktopPropertyPalette(parent, rightPalette)
{
    // Setup color choice widget
    QList<QAction *> colorActions;
    colorActions.append(WBApplication::mainWindow->actionColor0);
    colorActions.append(WBApplication::mainWindow->actionColor1);
    colorActions.append(WBApplication::mainWindow->actionColor2);
    colorActions.append(WBApplication::mainWindow->actionColor3);
    colorActions.append(WBApplication::mainWindow->actionColor4);

    WBToolbarButtonGroup *colorChoice =
            new WBToolbarButtonGroup(WBApplication::mainWindow->boardToolBar, colorActions);

    colorChoice->displayText(false);

    //connect(colorChoice, SIGNAL(activated(int)), this, SLOT(WBApplication::boardController->setColorIndex(int)));
    connect(WBDrawingController::drawingController(), SIGNAL(colorIndexChanged(int)), colorChoice, SLOT(setCurrentIndex(int)));
    connect(WBDrawingController::drawingController(), SIGNAL(colorIndexChanged(int)), this, SLOT(close()));
    connect(WBDrawingController::drawingController(), SIGNAL(colorPaletteChanged()), colorChoice, SLOT(colorPaletteChanged()));
    connect(WBDrawingController::drawingController(), SIGNAL(colorPaletteChanged()), this, SLOT(close()));

    layout()->addWidget(colorChoice);

    // Setup line width choice widget
    QList<QAction *> lineWidthActions;
    lineWidthActions.append(WBApplication::mainWindow->actionLineSmall);
    lineWidthActions.append(WBApplication::mainWindow->actionLineMedium);
    lineWidthActions.append(WBApplication::mainWindow->actionLineLarge);

    WBToolbarButtonGroup *lineWidthChoice =
            new WBToolbarButtonGroup(WBApplication::mainWindow->boardToolBar, lineWidthActions);
    lineWidthChoice->displayText(false);

    connect(lineWidthChoice, SIGNAL(activated(int)), WBDrawingController::drawingController(), SLOT(setLineWidthIndex(int)));
    connect(lineWidthChoice, SIGNAL(activated(int)), this, SLOT(close()));
    connect(WBDrawingController::drawingController(), SIGNAL(lineWidthIndexChanged(int)), lineWidthChoice, SLOT(setCurrentIndex(int)));
    connect(WBDrawingController::drawingController(), SIGNAL(lineWidthIndexChanged(int)), this, SLOT(close()));

    onParentMaximized();

    layout()->addWidget(lineWidthChoice);
}


void WBDesktopPenPalette::onButtonReleased()
{
    close();
}

/**
 * \brief Disconnect the released event of the buttons
 */
void WBDesktopPenPalette::onParentMinimized()
{
    for(int i = 0; i < mButtons.size(); i++)
    {
        disconnect(mButtons.at(i), SIGNAL(released()), this, SLOT(onButtonReleased()));
    }
}

/**
 * \brief Connect the released event of the buttons
 */
void WBDesktopPenPalette::onParentMaximized()
{
    for(int i = 0; i < mButtons.size(); i++)
    {
        connect(mButtons.at(i), SIGNAL(released()), this, SLOT(onButtonReleased()));
    }
}


WBDesktopEraserPalette::WBDesktopEraserPalette(QWidget *parent, WBRightPalette* rightPalette)
    : WBDesktopPropertyPalette(parent, rightPalette)
{
    // Setup eraser width choice widget
    QList<QAction *> eraserWidthActions;
    eraserWidthActions.append(WBApplication::mainWindow->actionEraserSmall);
    eraserWidthActions.append(WBApplication::mainWindow->actionEraserMedium);
    eraserWidthActions.append(WBApplication::mainWindow->actionEraserLarge);

    WBToolbarButtonGroup *eraserWidthChoice = new WBToolbarButtonGroup(WBApplication::mainWindow->boardToolBar, eraserWidthActions);

    connect(eraserWidthChoice, SIGNAL(activated(int)), WBDrawingController::drawingController(), SLOT(setEraserWidthIndex(int)));
    connect(eraserWidthChoice, SIGNAL(activated(int)), this, SLOT(close()));
    connect(WBApplication::mainWindow->actionEraseDesktopAnnotations, SIGNAL(triggered()), this, SLOT(close()));

    eraserWidthChoice->displayText(false);
    eraserWidthChoice->setCurrentIndex(WBSettings::settings()->eraserWidthIndex());

    layout()->addWidget(eraserWidthChoice);

    addAction(WBApplication::mainWindow->actionEraseDesktopAnnotations);
}


WBDesktopMarkerPalette::WBDesktopMarkerPalette(QWidget *parent, WBRightPalette* rightPalette)
    : WBDesktopPropertyPalette(parent, rightPalette)
{
    // Setup color choice widget
    QList<QAction *> colorActions;
    colorActions.append(WBApplication::mainWindow->actionColor0);
    colorActions.append(WBApplication::mainWindow->actionColor1);
    colorActions.append(WBApplication::mainWindow->actionColor2);
    colorActions.append(WBApplication::mainWindow->actionColor3);
    colorActions.append(WBApplication::mainWindow->actionColor4);

    WBToolbarButtonGroup *colorChoice = new WBToolbarButtonGroup(WBApplication::mainWindow->boardToolBar, colorActions);
    colorChoice->displayText(false);

    //connect(colorChoice, SIGNAL(activated(int)), this, SLOT(WBApplication::boardController->setColorIndex(int)));
    connect(WBDrawingController::drawingController(), SIGNAL(colorIndexChanged(int)), colorChoice, SLOT(setCurrentIndex(int)));
    connect(WBDrawingController::drawingController(), SIGNAL(colorIndexChanged(int)), this, SLOT(close()));
    connect(WBDrawingController::drawingController(), SIGNAL(colorPaletteChanged()), colorChoice, SLOT(colorPaletteChanged()));
    connect(WBDrawingController::drawingController(), SIGNAL(colorPaletteChanged()), this, SLOT(close()));

    layout()->addWidget(colorChoice);

    // Setup line width choice widget
    QList<QAction *> lineWidthActions;
    lineWidthActions.append(WBApplication::mainWindow->actionLineSmall);
    lineWidthActions.append(WBApplication::mainWindow->actionLineMedium);
    lineWidthActions.append(WBApplication::mainWindow->actionLineLarge);

    WBToolbarButtonGroup *lineWidthChoice = new WBToolbarButtonGroup(WBApplication::mainWindow->boardToolBar, lineWidthActions);
    lineWidthChoice->displayText(false);

    connect(lineWidthChoice, SIGNAL(activated(int)), WBDrawingController::drawingController(), SLOT(setLineWidthIndex(int)));
    connect(lineWidthChoice, SIGNAL(activated(int)), this, SLOT(close()));
    connect(WBDrawingController::drawingController(), SIGNAL(lineWidthIndexChanged(int)), lineWidthChoice, SLOT(setCurrentIndex(int)));
    connect(WBDrawingController::drawingController(), SIGNAL(lineWidthIndexChanged(int)), this, SLOT(close()));

    layout()->addWidget(lineWidthChoice);
}

