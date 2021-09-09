#ifndef WBMESSAGEWINDOW_H_
#define WBMESSAGEWINDOW_H_

#include <QtWidgets>
#include <QHBoxLayout>
#include <QLabel>

#include "WBFloatingPalette.h"

class WBSpinningWheel;

class WBMessageWindow  : public WBFloatingPalette
{
    Q_OBJECT

    public:
        WBMessageWindow(QWidget *parent = 0);
        virtual ~WBMessageWindow();

        void showMessage(const QString& message, bool showSpinningWheel = false);
        void hideMessage();

    protected:
        void timerEvent(QTimerEvent *event);

    private:
        QHBoxLayout *mLayout;
        WBSpinningWheel *mSpinningWheel;
        QLabel *mLabel;
        QBasicTimer  mTimer;

        int mOriginalAlpha;
        int mFadingStep;

        int mTimerID;
};

#endif /* WBMESSAGEWINDOW_H_ */
