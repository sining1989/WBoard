#ifndef WBDRAWINGCONTROLLER_H_
#define WBDRAWINGCONTROLLER_H_

#include <QtCore>

#include "core/WB.h"
#include "gui/WBStylusPalette.h"

class WBAbstractDrawRuler;
class WBDrawingController : public QObject
{
    Q_OBJECT

    private:
        WBDrawingController(QObject * parent = 0);
        virtual ~WBDrawingController();

    public:
        static WBDrawingController* drawingController();
        static void destroy();

        int stylusTool();
        int latestDrawingTool();

        bool isDrawingTool();

        int currentToolWidthIndex();
        qreal currentToolWidth();
        int currentToolColorIndex();
        QColor currentToolColor();
        QColor toolColor(bool onDarkBackground);

        void setPenColor(bool onDarkBackground, const QColor& color, int pIndex);
        void setMarkerColor(bool onDarkBackground, const QColor& color, int pIndex);
        void setMarkerAlpha(qreal alpha);

        WBAbstractDrawRuler* mActiveRuler;

        void setInDestopMode(bool mode){
            mIsDesktopMode = mode;
        }

        bool isInDesktopMode(){
            return mIsDesktopMode;
        }
		
    public slots:
        void setStylusTool(int tool);
        void setLineWidthIndex(int index);
        void setColorIndex(int index);
        void setEraserWidthIndex(int index);

    signals:
        void stylusToolChanged(int tool);
        void colorPaletteChanged();

        void lineWidthIndexChanged(int index);
        void colorIndexChanged(int index);

    private:
        WBStylusTool::Enum mStylusTool;
        WBStylusTool::Enum mLatestDrawingTool;
        bool mIsDesktopMode;

        static WBDrawingController* sDrawingController;

    private slots:
        void penToolSelected(bool checked);
        void eraserToolSelected(bool checked);
        void markerToolSelected(bool checked);
        void selectorToolSelected(bool checked);
        void playToolSelected(bool checked);
        void handToolSelected(bool checked);
        void zoomInToolSelected(bool checked);
        void zoomOutToolSelected(bool checked);
        void pointerToolSelected(bool checked);
        void lineToolSelected(bool checked);
        void textToolSelected(bool checked);
        void captureToolSelected(bool checked);
};

#endif /* WBDRAWINGCONTROLLER_H_ */
