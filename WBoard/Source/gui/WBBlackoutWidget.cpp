#include <QtWidgets>

#include "WBBlackoutWidget.h"

#include "core/memcheck.h"

WBBlackoutWidget::WBBlackoutWidget(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
}


void WBBlackoutWidget::mousePressEvent(QMouseEvent *event)
{
        Q_UNUSED(event);
    doActivity();
}


void WBBlackoutWidget::keyPressEvent(QKeyEvent *event)
{
    if (!event->isAccepted())
    {
        if (event->key() == Qt::Key_B)
        {
            doActivity();
            event->accept();
        }
        else
        {
            event->ignore();
        }
    }
}


void WBBlackoutWidget::doActivity()
{
    emit activity();
}
