#include <QtWidgets>

#include "WBIconButton.h"

#include "core/memcheck.h"


WBIconButton::WBIconButton(QWidget *parent, const QIcon &icon)
    : QAbstractButton(parent)
    , mToggleable(false)
{
    setIcon(icon);
    setCheckable(true);
    mIconSize = icon.actualSize(QSize(128, 128));
}

void WBIconButton::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    QPixmap pixmap = icon().pixmap(mIconSize, isChecked() ? QIcon::Selected : QIcon::Normal);
    painter.drawPixmap((width() - pixmap.width()) / 2, 0, pixmap);
}

void WBIconButton::mousePressEvent(QMouseEvent *event)
{
    if (!mToggleable)
        setChecked(true);

    QAbstractButton::mousePressEvent(event);
}

void WBIconButton::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event);

    emit doubleClicked();
}

QSize WBIconButton::minimumSizeHint() const
{
    return mIconSize;
}
