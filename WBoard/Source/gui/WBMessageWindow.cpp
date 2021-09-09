#include "WBMessageWindow.h"

#include "WBSpinningWheel.h"

#include "core/memcheck.h"

WBMessageWindow::WBMessageWindow(QWidget *parent)
    : WBFloatingPalette(Qt::BottomLeftCorner, parent)
    , mTimerID(-1)
{
    mLayout = new QHBoxLayout(this);
    mSpinningWheel = new WBSpinningWheel(parent);
    mLabel = new QLabel(parent);
    mLabel->setStyleSheet(QString("QLabel { color: white; background-color: transparent; border: none; font-family: Arial; font-size: 14px }"));

    mOriginalAlpha = 255;

    mLayout->setContentsMargins(radius() + 15, 4, radius() + 15, 4);

#ifdef Q_OS_OSX
    mLayout->setContentsMargins(radius() + 15, 8, radius() + 15, 10);
#endif

    mLayout->addWidget(mSpinningWheel);
    mLayout->addWidget(mLabel);
}

WBMessageWindow::~WBMessageWindow()
{
    // NOOP
}

void WBMessageWindow::showMessage(const QString& message, bool showSpinningWheel)
{
    mTimer.stop();

    mLabel->setText(message);

    QColor fadedColor = mBackgroundBrush.color();
    fadedColor.setAlpha(mOriginalAlpha);
    mBackgroundBrush = QBrush(fadedColor);
    mFadingStep = 25;

    if (showSpinningWheel)
    {
        mSpinningWheel->show();
        mSpinningWheel->startAnimation();
    }
    else
    {
        mSpinningWheel->hide();
        mSpinningWheel->stopAnimation();
        mTimer.start(50, this);
    }
    adjustSizeAndPosition();

    show();
    // showMessage may have been called from the GUI thread, so make sure the message window is drawn right now
    repaint();
    // I mean it, *right now*, also on Mac
    qApp->flush();
    //qApp->sendPostedEvents();
}

void WBMessageWindow::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event);

    mFadingStep--;

    if (mFadingStep < 1)
    {
        hide();
        mTimer.stop();
        mSpinningWheel->stopAnimation();
    }
    else if (mFadingStep < 10)
    {
        QColor fadedColor = mBackgroundBrush.color();
        fadedColor.setAlpha(mOriginalAlpha / 10 * mFadingStep);
        mBackgroundBrush = QBrush(fadedColor);

        update();
    }
}

void WBMessageWindow::hideMessage()
{
    mFadingStep = 0;
    hide();
    qApp->flush();
}
