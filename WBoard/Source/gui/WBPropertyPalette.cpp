#include "WBPropertyPalette.h"

#include "core/memcheck.h"

WBPropertyPalette::WBPropertyPalette(QWidget *parent, const char *name):WBActionPalette(parent)
{
    setObjectName(name);
    mbGrip = false;
}


WBPropertyPalette::WBPropertyPalette(Qt::Orientation orientation, QWidget *parent):WBActionPalette(orientation, parent)
{
    mbGrip = false;
}

WBPropertyPalette::~WBPropertyPalette()
{

}

void WBPropertyPalette::onMouseRelease()
{
//    qDebug() << "WBPropertyPalette::onMouseRelease() called (" << mMousePos.x() << "," << mMousePos.y();
//    QWidget* pW = NULL;
//    pW = childAt(mMousePos);

//    if(NULL != pW)
//    {
//        // A widget has been found under the mouse!
//        WBActionPaletteButton* pButton = dynamic_cast<WBActionPaletteButton*>(pW);
//        if(NULL != pButton)
//        {
//            pButton->click();
//        }
//    }

//    // Close the palette
//    close();
}
