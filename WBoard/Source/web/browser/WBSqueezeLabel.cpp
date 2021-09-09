#include "WBSqueezeLabel.h"

#include "core/memcheck.h"

WBSqueezeLabel::WBSqueezeLabel(QWidget *parent) : QLabel(parent)
{
}

void WBSqueezeLabel::paintEvent(QPaintEvent *event)
{
    QFontMetrics fm = fontMetrics();
    if (fm.width(text()) > contentsRect().width()) {
        QString elided = fm.elidedText(text(), Qt::ElideMiddle, width());
        QString oldText = text();
        setText(elided);
        QLabel::paintEvent(event);
        setText(oldText);
    } else {
        QLabel::paintEvent(event);
    }
}

