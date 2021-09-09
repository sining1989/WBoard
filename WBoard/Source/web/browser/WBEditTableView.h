#ifndef WBEDITTABLEVIEW_H
#define WBEDITTABLEVIEW_H

#include <QtWidgets>
#include <QTableView>

class WBEditTableView : public QTableView
{
    Q_OBJECT;

public:
    WBEditTableView(QWidget *parent = 0);
    void keyPressEvent(QKeyEvent *event);

public slots:
    void removeOne();
    void removeAll();
};

#endif // WBEDITTABLEVIEW_H

