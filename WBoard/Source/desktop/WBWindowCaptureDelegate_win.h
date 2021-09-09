#ifndef WBWINDOWCAPTUREDELEGATE_H_
#define WBWINDOWCAPTUREDELEGATE_H_

#include <QtWidgets>

class WBWindowCaptureDelegate : public QObject
{
    Q_OBJECT

    public:
        WBWindowCaptureDelegate(QObject *parent = 0);
        ~WBWindowCaptureDelegate();

        int execute();
        const QPixmap getCapturedWindow();

    private:
        bool eventFilter(QObject *target, QEvent *event);
        void processPos(QPoint pPoint);
        void drawSelectionRect();

        bool mIsCapturing;
        bool mCancel;
        QPoint mLastPoint;
        HWND mCurrentWindow;
        QPixmap mCapturedPixmap;

};
#endif /* WBWINDOWCAPTUREDELEGATE_H_ */
