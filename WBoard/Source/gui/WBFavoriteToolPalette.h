#ifndef WBFAVORITETOOLPALETTE_H_
#define WBFAVORITETOOLPALETTE_H_

#include <QtWidgets>

#include "WBActionPalette.h"

class WBFavoriteToolPalette : public WBActionPalette
{
    Q_OBJECT

    public:
        WBFavoriteToolPalette(QWidget* parent = 0);
        virtual ~WBFavoriteToolPalette();

    private slots:
        void addFavorite();
};

#endif /* WBFAVORITETOOLPALETTE_H_ */
