#ifndef WBLEFTPALETTE_H
#define WBLEFTPALETTE_H

#include "WBDockPalette.h"

class WBLeftPalette : public WBDockPalette
{
public:
    WBLeftPalette(QWidget* parent=0, const char* name="WBLeftPalette");
    ~WBLeftPalette();

    bool switchMode(eWBDockPaletteWidgetMode mode);

public slots:
    void onDocumentSet(WBDocumentProxy* documentProxy);

protected:
    void updateMaxWidth();
    void resizeEvent(QResizeEvent *event);
};

#endif // WBLEFTPALETTE_H
