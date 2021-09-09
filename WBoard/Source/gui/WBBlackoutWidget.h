#ifndef WBBLACKOUTWIDGET_H_
#define WBBLACKOUTWIDGET_H_

#include <QWidget>

class WBBlackoutWidget : public QWidget
{
    Q_OBJECT

    public:
        WBBlackoutWidget(QWidget *parent = 0);
        virtual void mousePressEvent(QMouseEvent *event);
        virtual void keyPressEvent(QKeyEvent *event);

    signals:
        void activity();

    public slots:
        void doActivity();
};

#endif /* WBBLACKOUTWIDGET_H_ */
