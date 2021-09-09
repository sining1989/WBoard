#include "WBDockDownloadWidget.h"
#include "core/WBApplication.h"

#include "globals/WBGlobals.h"

#include "core/memcheck.h"

WBDockDownloadWidget::WBDockDownloadWidget(QWidget *parent, const char *name):WBDockPaletteWidget(parent, name)
  , mpLayout(NULL)
  , mpDLWidget(NULL)
{
    mName = "DownloadWidget";
    mVisibleState = false;

    SET_STYLE_SHEET();

    mIconToLeft = QPixmap(":images/general_open.png");
    mIconToRight = QPixmap(":images/general_close.png");

    mpLayout = new QVBoxLayout(this);
    setLayout(mpLayout);

    mpDLWidget = new WBDownloadWidget(this);
    mpLayout->addWidget(mpDLWidget);
}

WBDockDownloadWidget::~WBDockDownloadWidget()
{
    if(NULL != mpDLWidget)
    {
        delete mpDLWidget;
        mpDLWidget = NULL;
    }
    if(NULL != mpLayout)
    {
        delete mpLayout;
        mpLayout = NULL;
    }
}
