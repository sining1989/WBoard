#ifndef WBUNINOTESWINDOWCONTROLLER_H_
#define WBUNINOTESWINDOWCONTROLLER_H_

#include <QtWidgets>
#include <QTime>
#include <QTimer>

#include "gui/WBRightPalette.h"

class WBDesktopPalette;
class WBBoardView;
class WBGraphicsScene;
class WBDesktopPenPalette;
class WBDesktopMarkerPalette;
class WBDesktopEraserPalette;
class WBActionPalette;
class WBMainWindow;
class WBRightPalette;

#define PROPERTY_PALETTE_TIMER      1000

class WBDesktopAnnotationController : public QObject
{
    Q_OBJECT

    public:
        WBDesktopAnnotationController(QObject *parent, WBRightPalette* rightPalette);
        virtual ~WBDesktopAnnotationController();
        void showWindow();
        void hideWindow();

        WBDesktopPalette *desktopPalette();
        QPainterPath desktopPalettePath() const;
        WBBoardView *drawingView();

        void TransparentWidgetResized();

    public slots:
        void screenLayoutChanged();
        void goToUniboard();
        void customCapture();
        void windowCapture();
        void screenCapture();
        void updateShowHideState(bool pEnabled);

        void close();

        void stylusToolChanged(int tool);
        void updateBackground();

//         void showKeyboard(bool show);
//         void showKeyboard(); //X11 virtual keyboard working only needed

    signals:
        void imageCaptured(const QPixmap& pCapturedPixmap, bool pageMode);
        void restoreUniboard();

    protected:
        QPixmap getScreenPixmap();

        WBBoardView* mTransparentDrawingView;       
        WBGraphicsScene* mTransparentDrawingScene;

    private slots:
        void updateColors();
        void desktopPenActionToggled(bool checked);
        void desktopMarkerActionToggled(bool checked);
        void desktopEraserActionToggled(bool checked);
        void eraseDesktopAnnotations();
        void penActionPressed();
        void markerActionPressed();
        void eraserActionPressed();
        void penActionReleased();
        void markerActionReleased();
        void eraserActionReleased();
        void selectorActionPressed();
        void selectorActionReleased();
        void pointerActionPressed();
        void pointerActionReleased();

        void switchCursor(int tool);
        void onDesktopPaletteMaximized();
        void onDesktopPaletteMinimize();
        void onTransparentWidgetResized();
        void refreshMask();
        void onToolClicked();

    private:
        void setAssociatedPalettePosition(WBActionPalette* palette, const QString& actionName);
        void togglePropertyPalette(WBActionPalette* palette);
        void updateMask(bool bTransparent);

        WBDesktopPalette *mDesktopPalette;
        //WBKeyboardPalette *mKeyboardPalette;
        WBDesktopPenPalette* mDesktopPenPalette;
        WBDesktopMarkerPalette* mDesktopMarkerPalette;
        WBDesktopEraserPalette* mDesktopEraserPalette;

        WBRightPalette* mRightPalette;

        QTime mPenHoldTimer;
        QTime mMarkerHoldTimer;
        QTime mEraserHoldTimer;
        QTimer mHoldTimerPen;
        QTimer mHoldTimerMarker;
        QTimer mHoldTimerEraser;

        bool mWindowPositionInitialized;
        bool mIsFullyTransparent;
        bool mDesktopToolsPalettePositioned;
        bool mPendingPenButtonPressed;
        bool mPendingMarkerButtonPressed;
        bool mPendingEraserButtonPressed;
        bool mbArrowClicked;

        int mBoardStylusTool;
        int mDesktopStylusTool;

        QPixmap mMask;

};

#endif /* WBUNINOTESWINDOWCONTROLLER_H_ */
