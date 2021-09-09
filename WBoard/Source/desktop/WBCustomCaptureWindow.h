#ifndef WBCUSTOMCAPTUREWINDOW_H_
#define WBCUSTOMCAPTUREWINDOW_H_

#include <QtWidgets>
#include <QDialog>
#include <QRubberBand>

class WBCustomCaptureWindow : public QDialog
{
    Q_OBJECT

    public:
        WBCustomCaptureWindow(QWidget *parent = 0);
        virtual ~WBCustomCaptureWindow();

        int execute(const QPixmap &pScreenPixmap);

        QPixmap getSelectedPixmap();

    protected:
        virtual void showEvent ( QShowEvent * event );
        virtual void mouseMoveEvent ( QMouseEvent * event );
        virtual void mousePressEvent ( QMouseEvent * event );
        virtual void mouseReleaseEvent ( QMouseEvent * event );
        virtual void keyPressEvent ( QKeyEvent * event );
        virtual void paintEvent(QPaintEvent *event);

        QPixmap mWholeScreenPixmap;
        QRubberBand *mSelectionBand;
        QStyle *mRubberBandStyle;
        QPoint mOrigin;
        bool mIsSelecting;
};

#endif /* WBCUSTOMCAPTUREWINDOW_H_ */
