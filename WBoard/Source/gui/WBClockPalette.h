#ifndef WBCLOCKPALLETTE_H_
#define WBCLOCKPALLETTE_H_

class QHBoxLayout;
class QLabel;

#include "WBFloatingPalette.h"

class WBClockPalette : public WBFloatingPalette
{
Q_OBJECT

public:
    WBClockPalette(QWidget *parent = 0);
    virtual ~WBClockPalette();

protected:
    int radius();
    void timerEvent(QTimerEvent *event);

    virtual void showEvent ( QShowEvent * event );
    virtual void hideEvent ( QShowEvent * event );

private:
    QLabel *mTimeLabel;
    int mTimerID;
    QString mTimeFormat;

};

#endif /* WBCLOCKPALLETTE_H_ */
