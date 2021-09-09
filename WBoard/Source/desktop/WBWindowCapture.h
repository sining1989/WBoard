#ifndef WBWINDOWCAPTURE_H_
#define WBWINDOWCAPTURE_H_

#include <QtWidgets>

class WBDesktopAnnotationController;

class WBWindowCapture : public QObject
{
    Q_OBJECT

    public:
        WBWindowCapture(WBDesktopAnnotationController *parent = 0);
        virtual ~WBWindowCapture();
        int execute();
        const QPixmap getCapturedWindow();

    private:
        QPixmap mWindowPixmap;
        WBDesktopAnnotationController* mParent;
};
#endif /* WBWINDOWCAPTURE_H_ */
