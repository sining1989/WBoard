#include <QDesktopWidget>

#include "WBScreenMirror.h"

#include "core/WBSettings.h"
#include "core/WBSetting.h"
#include "core/WBApplication.h"
#include "board/WBBoardController.h"

#if defined(Q_OS_OSX)
#include <ApplicationServices/ApplicationServices.h>
#endif

#include "core/memcheck.h"


WBScreenMirror::WBScreenMirror(QWidget* parent)
    : QWidget(parent)
    , mScreenIndex(0)
    , mSourceWidget(0)
    , mTimerID(0)
{
    // NOOP
}


WBScreenMirror::~WBScreenMirror()
{
    // NOOP
}


void WBScreenMirror::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);

    painter.fillRect(0, 0, width(), height(), QBrush(Qt::black));

    if (!mLastPixmap.isNull())
    {
        int x = (width() - mLastPixmap.width()) / 2;
        int y = (height() - mLastPixmap.height()) / 2;

        painter.drawPixmap(x, y, mLastPixmap);
    }
}


void WBScreenMirror::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event);

    grabPixmap();

    update();
}

void WBScreenMirror::grabPixmap()
{
    if (mSourceWidget)
    {
        QPoint topLeft = mSourceWidget->mapToGlobal(mSourceWidget->geometry().topLeft());
        QPoint bottomRight = mSourceWidget->mapToGlobal(mSourceWidget->geometry().bottomRight());

        mRect.setTopLeft(topLeft);
        mRect.setBottomRight(bottomRight);
        mLastPixmap = mSourceWidget->grab();
    }
    else{
        // WHY HERE?
        // this is the case we are showing the desktop but the is no widget and we use the last widget rectagle to know
        // what we have to grab. Not very good way of doing
        QDesktopWidget * desktop = QApplication::desktop();
        QScreen * screen = WBApplication::controlScreen();
        mLastPixmap = screen->grabWindow(desktop->effectiveWinId(), mRect.x(), mRect.y(), mRect.width(), mRect.height());
    }

    if (!mLastPixmap.isNull())
        mLastPixmap = mLastPixmap.scaled(width(), height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
}


void WBScreenMirror::setSourceWidget(QWidget *sourceWidget)
{
    mSourceWidget = sourceWidget;

    mScreenIndex = qApp->desktop()->screenNumber(sourceWidget);

    grabPixmap();

    update();
}


void WBScreenMirror::start()
{
    qDebug() << "mirroring START";
    WBApplication::boardController->freezeW3CWidgets(true);
    if (mTimerID == 0)
    {
        int ms = 125;

        bool success;
        int fps = WBSettings::settings()->mirroringRefreshRateInFps->get().toInt(&success);

        if (success && fps > 0)
        {
            ms = 1000 / fps;
        }

        mTimerID = startTimer(ms);
    }
    else
    {
        qDebug() << "WBScreenMirror::start() : Timer already running ...";
    }
}


void WBScreenMirror::stop()
{
    qDebug() << "mirroring STOP";
    WBApplication::boardController->freezeW3CWidgets(false);
    if (mTimerID != 0)
    {
        killTimer(mTimerID);
        mTimerID = 0;
    }
}
