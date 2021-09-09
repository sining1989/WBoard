#include <QtWidgets>
#include <QPainterPath>

#include "WBFloatingPalette.h"

#include "frameworks/WBPlatformUtils.h"

#include "core/WBSettings.h"

#include "core/memcheck.h"

WBFloatingPalette::WBFloatingPalette(Qt::Corner position, QWidget *parent)
    : QWidget(parent, parent ? Qt::Widget : Qt::Tool | (Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint))
    , mCustomPosition(false)
    , mIsMoving(false)
    , mCanBeMinimized(false)
    , mMinimizedLocation(eMinimizedLocation_None)
    , mDefaultPosition(position)
{
    setCursor(Qt::ArrowCursor);

    if (parent)
    {
        setAttribute(Qt::WA_NoMousePropagation);
    }
    else
    {
        // standalone window
        // !!!! Should be included into Windows after QT recompilation
#ifndef Q_OS_WIN
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_MacAlwaysShowToolWindow);
#endif
#ifdef Q_OS_OSX
        setAttribute(Qt::WA_MacAlwaysShowToolWindow);
        setAttribute(Qt::WA_MacNoShadow);
#endif
    }

    mBackgroundBrush = QBrush(WBSettings::paletteColor);
    mbGrip = true;
}

void WBFloatingPalette::setGrip(bool newGrip)
{
    mbGrip = newGrip;
    update();
}


void WBFloatingPalette::setBackgroundBrush(const QBrush& brush)
{
    if (mBackgroundBrush != brush)
    {
        mBackgroundBrush = brush;
        update();
    }
}


void WBFloatingPalette::setCustomPosition(bool pFlag)
{
    mCustomPosition = pFlag;

    if (pFlag)
        removeAllAssociatedPalette();

}

int WBFloatingPalette::radius()
{
    return 10;
}


int WBFloatingPalette::border()
{
    return 0;
}


void WBFloatingPalette::enterEvent(QEvent *event)
{
    Q_UNUSED(event);
    emit mouseEntered();
}

void WBFloatingPalette::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        mIsMoving = true;
        mDragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
    else
    {
        QWidget::mousePressEvent(event);
    }
}

void WBFloatingPalette::mouseMoveEvent(QMouseEvent *event)
{
    if (mIsMoving)
    {
        moveInsideParent(event->globalPos() - mDragPosition);
        event->accept();
        emit moving();
    }
    else
    {
        QWidget::mouseMoveEvent(event);
    }
}

void WBFloatingPalette::mouseReleaseEvent(QMouseEvent *event)
{
    if (mIsMoving)
    {
        mIsMoving = false;
        setCustomPosition(true);
        event->accept();
    }
    else
    {
        QWidget::mouseReleaseEvent(event);
    }
}

int WBFloatingPalette::getParentRightOffset()
{
    return 0;
}

void WBFloatingPalette::moveInsideParent(const QPoint &position)
{
    QWidget *parent = parentWidget();

    if (parent)
    {
        int margin = WBSettings::boardMargin - border();
        qreal newX = qMax(margin, qMin(parent->width() - getParentRightOffset() - width() - margin, position.x()));
        qreal newY = qMax(margin, qMin(parent->height() - height() - margin, position.y()));

        if (!mCustomPosition && !mIsMoving)
        {
            if (mDefaultPosition == Qt::TopLeftCorner || mDefaultPosition == Qt::BottomLeftCorner)
            {
                newX = margin;
            }
            else
            {
                newX = qMax(margin, parent->width() - getParentRightOffset() - width() - margin);
            }
        }
        move(newX, newY);
        minimizePalette(QPoint(newX, newY));
    }
    else
    {
        move(position);
    }
}

void WBFloatingPalette::addAssociatedPalette(WBFloatingPalette* pOtherPalette)
{
    mAssociatedPalette.append(pOtherPalette);
}

void WBFloatingPalette::removeAssociatedPalette(WBFloatingPalette* pOtherPalette)
{
    mAssociatedPalette.removeOne(pOtherPalette);
}

QSize WBFloatingPalette::preferredSize()
{
    QSize palettePreferredSize = sizeHint();

    if (palettePreferredSize.width() == 0)
    {
        palettePreferredSize = childrenRect().size();
    }

    return palettePreferredSize;
}

void WBFloatingPalette::adjustSizeAndPosition(bool pUp)
{
    QSize newPreferredSize = preferredSize();

    foreach (WBFloatingPalette* palette, mAssociatedPalette)
    {
        QSize palettePreferredSize = palette->preferredSize();
        newPreferredSize.setWidth(newPreferredSize.expandedTo(palettePreferredSize).width());
    }
    QSize previousSize = size();
    int biggerHeight = preferredSize().height() - previousSize.height();
    if ((pUp && (biggerHeight > 0))
        || (!pUp && (biggerHeight < 0)))
    {
        move(pos().x(), pos().y() - biggerHeight);
    }

    if (newPreferredSize != size())
    {
        resize(newPreferredSize);
        moveInsideParent(pos());
        foreach(WBFloatingPalette* palette, mAssociatedPalette)
        {
            palette->move(pos().x(), palette->pos().y());
            palette->resize(newPreferredSize.width(), palette->size().height());
        }
    }
}

void WBFloatingPalette::removeAllAssociatedPalette()
{
    foreach (WBFloatingPalette* palette, mAssociatedPalette)
    {
        palette->removeAssociatedPalette(this);
        removeAssociatedPalette(palette);
    }
}

void WBFloatingPalette::showEvent(QShowEvent *)
{
    moveInsideParent(pos());
}

void WBFloatingPalette::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(mBackgroundBrush);

    if(mbGrip)
    {
        painter.setBrush(QBrush(QColor(170, 170 ,170)));
        QPainterPath borderPath;
        borderPath.addRoundedRect(0, 0, width(), height(), radius(), radius());
        borderPath.addRoundedRect(border(), border(), width() - 2 * border(), height() - 2 * border(), radius(), radius());
        painter.drawPath(borderPath);
        painter.setBrush(mBackgroundBrush);
        painter.drawRoundedRect(border(), border(), width() - 2 * border(), height() - 2 * border(), radius(), radius());
    }
    else
    {
        painter.drawRoundedRect(border(), border(), width() - 2 * border(), height() - 2 * border(), radius(), radius());
    }
}

int WBFloatingPalette::gripSize()
{
    return 5;
}

void WBFloatingPalette::minimizePalette(const QPoint& pos)
{
    if(!mCanBeMinimized)
    {
        //  If this floating palette cannot be minimized, we exit this method.
    return;
    }

    if(mMinimizedLocation == eMinimizedLocation_None)
    {
    //  Verify if we have to minimize this palette
    if(pos.x() == 5)
    {
        mMinimizedLocation = eMinimizedLocation_Left;
    }
//    else if(pos.y() == 5)
//    {
//        mMinimizedLocation = eMinimizedLocation_Top;
//    }
    else if(pos.x() == parentWidget()->width() - getParentRightOffset() - width() - 5)
    {
        mMinimizedLocation = eMinimizedLocation_Right;
    }
//    else if(pos.y() == parentSize.height() - height() - 5)
//    {
//        mMinimizedLocation = eMinimizedLocation_Bottom;
//    }

    //  Minimize the Palette
    if(mMinimizedLocation != eMinimizedLocation_None)
    {
        emit minimizeStart(mMinimizedLocation);
    }
    }
    else
    {
    //  Restore the palette
    if(pos.x() > 5 &&
       pos.y() > 5 &&
       pos.x() < parentWidget()->width() - getParentRightOffset()  - width() - 5 &&
       pos.y() < parentWidget()->size().height() - height() - 5)
    {
        mMinimizedLocation = eMinimizedLocation_None;
        emit maximizeStart();
    }
    }
}

void WBFloatingPalette::setMinimizePermission(bool permission)
{
    mCanBeMinimized = permission;
}
