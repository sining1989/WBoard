#include "WBColorPicker.h"

#include <QtWidgets>

#include "core/memcheck.h"

WBColorPicker::WBColorPicker(QWidget* parent)
    : QFrame(parent)
    , mSelectedColorIndex(0)
{
    // NOOP
}

WBColorPicker::WBColorPicker(QWidget* parent, const QList<QColor>& colors, int pSelectedColorIndex)
    : QFrame(parent)
    , mColors(colors)
    , mSelectedColorIndex(pSelectedColorIndex)
{
    // NOOP
}


WBColorPicker::~WBColorPicker()
{
    // NOOP
}


void WBColorPicker::paintEvent ( QPaintEvent * event )
{
    Q_UNUSED(event);

    QPainter painter(this);

    if (mSelectedColorIndex < mColors.size())
    {
        painter.setRenderHint(QPainter::Antialiasing);

        painter.setBrush(mColors.at(mSelectedColorIndex));

        painter.drawRect(0, 0, width(), height());
    }

}

void WBColorPicker::mousePressEvent ( QMouseEvent * event )
{
    if (event->buttons() & Qt::LeftButton)
    {
        mSelectedColorIndex++;

        if (mSelectedColorIndex >= mColors.size())
            mSelectedColorIndex = 0;

        repaint();

        emit colorSelected(mColors.at(mSelectedColorIndex));

    }
}


