#include "WBStylusPalette.h"

#include <QtWidgets>

#include "WBMainWindow.h"

#include "core/WBApplication.h"
#include "core/WBSettings.h"
#include "core/WBApplicationController.h"

#include "board/WBDrawingController.h"

#include "frameworks/WBPlatformUtils.h"

#include "core/memcheck.h"

WBStylusPalette::WBStylusPalette(QWidget *parent, Qt::Orientation orient)
    : WBActionPalette(Qt::TopLeftCorner, parent, orient)
    , mLastSelectedId(-1)
{
    QList<QAction*> actions;

    actions << WBApplication::mainWindow->actionPen;
    actions << WBApplication::mainWindow->actionEraser;
    actions << WBApplication::mainWindow->actionMarker;
    actions << WBApplication::mainWindow->actionSelector;
    actions << WBApplication::mainWindow->actionPlay;

    actions << WBApplication::mainWindow->actionHand;
    actions << WBApplication::mainWindow->actionZoomIn;
    actions << WBApplication::mainWindow->actionZoomOut;

    actions << WBApplication::mainWindow->actionPointer;
    actions << WBApplication::mainWindow->actionLine;
    actions << WBApplication::mainWindow->actionText;
    actions << WBApplication::mainWindow->actionCapture;

    if(WBPlatformUtils::hasVirtualKeyboard())
        actions << WBApplication::mainWindow->actionVirtualKeyboard;

    setActions(actions);
    setButtonIconSize(QSize(42, 42));

    if(!WBPlatformUtils::hasVirtualKeyboard())
    {
        groupActions();
    }
    else
    {
        // VirtualKeyboard action is not in group
        // So, groupping all buttons, except last
        mButtonGroup = new QButtonGroup(this);
        for(int i=0; i < mButtons.size()-1; i++)
        {
            mButtonGroup->addButton(mButtons[i], i);
        }
        connect(mButtonGroup, SIGNAL(buttonClicked(int)), this, SIGNAL(buttonGroupClicked(int)));
    }

    adjustSizeAndPosition();

    initPosition();

    foreach(const WBActionPaletteButton* button, mButtons)
    {
        connect(button, SIGNAL(doubleClicked()), this, SLOT(stylusToolDoubleClicked()));
    }

}

void WBStylusPalette::initPosition()
{
    QWidget* pParentW = parentWidget();
    if(!pParentW) 
		return ;

    mCustomPosition = true;

    QPoint pos;
    int parentWidth = pParentW->width();
    int parentHeight = pParentW->height();

    if(WBSettings::settings()->appToolBarOrientationVertical->get().toBool()){
        int posX = border();
        int posY = (parentHeight / 2) - (height() / 2);
        pos.setX(posX);
        pos.setY(posY);
    }
    else {
        int posX = (parentWidth / 2) - (width() / 2);
        int posY = parentHeight - border() - height();
        pos.setX(posX);
        pos.setY(posY);
    }
    moveInsideParent(pos);
}

WBStylusPalette::~WBStylusPalette()
{

}

void WBStylusPalette::stylusToolDoubleClicked()
{
    emit stylusToolDoubleClicked(mButtonGroup->checkedId());
}

