#ifndef WBSQUEEZELABEL_H
#define WBSQUEEZELABEL_H

#include <QLabel>

class WBSqueezeLabel : public QLabel
{
    Q_OBJECT;

public:
    WBSqueezeLabel(QWidget *parent = 0);

protected:
    void paintEvent(QPaintEvent *event);

};

#endif // WBSQUEEZELABEL_H

