#include "WBThumbnailView.h"
#include "domain/WBGraphicsScene.h"

#include "core/WBMimeData.h"

#include "core/memcheck.h"

WBThumbnailView::WBThumbnailView(WBGraphicsScene *scene, QWidget* parent)
    : QGraphicsView(scene, parent)
    , mHBoxLayout(new QHBoxLayout(this))
{
    setAcceptDrops(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);

    int nominalWidth = scene->nominalSize().width();
    int nominalHeight = scene->nominalSize().height();
    QRectF nominalSceneRect(-nominalWidth/2, -nominalHeight/2, nominalWidth, nominalHeight);
    fitInView(nominalSceneRect, Qt::KeepAspectRatio);
    setSceneRect(nominalSceneRect);

    setStyleSheet( "QGraphicsView { border-style: none; }" );

    setFrameStyle(QFrame::Sunken | QFrame::StyledPanel);

    //set stylesheet
    setObjectName("DockPaletteWidgetBox");
    setStyleSheet("background:white");

    mHBoxLayout->setAlignment(Qt::AlignHCenter);
    setLayout(mHBoxLayout);
}
