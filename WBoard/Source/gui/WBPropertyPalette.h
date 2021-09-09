#ifndef WBPROPERTYPALETTE_H
#define WBPROPERTYPALETTE_H

#include <QMouseEvent>
#include <QPoint>

#include "WBActionPalette.h"

class WBPropertyPalette : public WBActionPalette
{
    Q_OBJECT
public:
    WBPropertyPalette(QWidget* parent=0, const char* name="propertyPalette");
    WBPropertyPalette(Qt::Orientation orientation, QWidget* parent = 0);
    ~WBPropertyPalette();

private slots:
    void onMouseRelease();
};

#endif // WBPROPERTYPALETTE_H
