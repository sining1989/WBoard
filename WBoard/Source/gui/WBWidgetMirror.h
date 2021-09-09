#ifndef WBWIDGETMIRROR_H_
#define WBWIDGETMIRROR_H_

#include <QtWidgets>
#include <QWidget>

class WBWidgetMirror : public QWidget
{
    Q_OBJECT

public:
    WBWidgetMirror(QWidget* sourceWidget, QWidget* parent = 0);
    virtual ~WBWidgetMirror();

public slots:
    void setSourceWidget(QWidget *sourceWidget);

protected:
    bool eventFilter(QObject *obj, QEvent *event);
    virtual void paintEvent ( QPaintEvent * event );

private:
    QWidget* mSourceWidget;

};

#endif /* WBWIDGETMIRROR_H_ */
