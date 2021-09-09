#include <QApplication>
#include <QDesktopWidget>
#include <QStyle>

#include "WBCustomCaptureWindow.h"

#include "frameworks/WBPlatformUtils.h"
#include "gui/WBRubberBand.h"

#include "core/memcheck.h"

WBCustomCaptureWindow::WBCustomCaptureWindow(QWidget *parent)
    : QDialog(parent, Qt::FramelessWindowHint  | Qt::Window)
    , mSelectionBand(0)
    , mRubberBandStyle(0)
    , mOrigin(0,0)
    , mIsSelecting(false)
{
    setCursor(Qt::CrossCursor);
    setWindowOpacity(0.0);
}


WBCustomCaptureWindow::~WBCustomCaptureWindow()
{
    delete mSelectionBand;
    delete mRubberBandStyle;
}


QPixmap WBCustomCaptureWindow::getSelectedPixmap()
{
    if (mSelectionBand)
    {
        return mWholeScreenPixmap.copy(mSelectionBand->geometry());
    }
    else
    {
        return QPixmap();
    }
}


int WBCustomCaptureWindow::execute(const QPixmap &pScreenPixmap)
{
    mWholeScreenPixmap = pScreenPixmap;

    QDesktopWidget *desktop = QApplication::desktop();
    int currentScreen = desktop->screenNumber(QCursor::pos());
    setGeometry(desktop->screenGeometry(currentScreen));
    this->show();
    setWindowOpacity(1.0);

    return exec();
}


void WBCustomCaptureWindow::mouseMoveEvent ( QMouseEvent * event )
{
    if (mIsSelecting)
    {
        mSelectionBand->setGeometry(QRect(mOrigin, event->pos()).normalized());
    }

    event->accept();
}


void WBCustomCaptureWindow::mousePressEvent ( QMouseEvent * event )
{
    if (!mIsSelecting)
    {
        mIsSelecting = true;
        mOrigin = event->pos();

        if (!mSelectionBand)
        {
            mSelectionBand = new WBRubberBand(QRubberBand::Rectangle, this);
        }

        mSelectionBand->setGeometry(QRect(mOrigin, QSize()));
        mSelectionBand->show();
        event->accept();
    }
}


void WBCustomCaptureWindow::mouseReleaseEvent ( QMouseEvent * event )
{
    mIsSelecting = false;

    if (mSelectionBand)
    {
        mSelectionBand->hide();
    }

    event->accept();

    // do not accept very small selection
    if (!(mSelectionBand->geometry().width() < 6 && mSelectionBand->geometry().height() < 6))
    {
        accept();
    }

}


void WBCustomCaptureWindow::keyPressEvent ( QKeyEvent * event )
{
    if (event->key() == Qt::Key_Escape)
    {
        mIsSelecting = false;

        if (mSelectionBand)
        {
            mSelectionBand->hide();
        }

        event->accept();
        reject();
    }
}


void WBCustomCaptureWindow::showEvent ( QShowEvent * event )
{
    Q_UNUSED(event);
}

void WBCustomCaptureWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.drawPixmap(0,0, mWholeScreenPixmap);
}
