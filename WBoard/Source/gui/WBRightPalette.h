#ifndef WBRIGHTPALETTE_H
#define WBRIGHTPALETTE_H

#include "WBDockPalette.h"

class WBRightPalette : public WBDockPalette
{
    Q_OBJECT
public:
    WBRightPalette(QWidget* parent=0, const char* name="WBRightPalette");
    ~WBRightPalette();
    bool switchMode(eWBDockPaletteWidgetMode mode);

signals:
    void resized();

protected:
    void updateMaxWidth();
    void mouseMoveEvent(QMouseEvent *event);
    void resizeEvent(QResizeEvent *event);

};

#endif // WBRIGHTPALETTE_H
