#ifndef WBDESKTOPPENPALETTE_H
#define WBDESKTOPPENPALETTE_H

#include <QtWidgets>
#include <QResizeEvent>

#include "gui/WBPropertyPalette.h"

class WBRightPalette;

class WBDesktopPropertyPalette : public WBPropertyPalette
{
    Q_OBJECT

    public:
        WBDesktopPropertyPalette(QWidget *parent, WBRightPalette* _rightPalette);
    private:
        WBRightPalette* rightPalette;
    protected:
        virtual int getParentRightOffset();
};

class WBDesktopPenPalette : public WBDesktopPropertyPalette
{
    Q_OBJECT
    public:
        WBDesktopPenPalette(QWidget *parent, WBRightPalette* rightPalette);
        virtual ~WBDesktopPenPalette(){}
    public slots:
        void onParentMinimized();
        void onParentMaximized();

    private slots:
        void onButtonReleased();

};

class WBDesktopEraserPalette : public WBDesktopPropertyPalette
{
    public:
        WBDesktopEraserPalette(QWidget *parent, WBRightPalette* rightPalette);
        virtual ~WBDesktopEraserPalette(){}
};

class WBDesktopMarkerPalette : public WBDesktopPropertyPalette
{
    public:
        WBDesktopMarkerPalette(QWidget *parent, WBRightPalette* rightPalette);
        virtual ~WBDesktopMarkerPalette(){}
};

#endif // WBDESKTOPPENPALETTE_H
