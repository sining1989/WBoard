#ifndef WBAUTOSAVER_H
#define WBAUTOSAVER_H

#include <QtCore>

class WBAutoSaver : public QObject {

    Q_OBJECT;

public:
    WBAutoSaver(QObject *parent);
    ~WBAutoSaver();
    void saveIfNeccessary();

public slots:
    void changeOccurred();

protected:
    void timerEvent(QTimerEvent *event);

private:
    QBasicTimer mTimer;
    QTime mFirstChange;

};

#endif // WBAUTOSAVER_H

