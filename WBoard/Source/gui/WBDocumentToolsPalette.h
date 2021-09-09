#ifndef WBDOCUMENTTOOLSPALLETTE_H_
#define WBDOCUMENTTOOLSPALLETTE_H_

#include <QButtonGroup>
#include <QUrl>
#include <QMap>

#include "WBActionPalette.h"

class WBDocumentToolsPalette : public WBActionPalette
{
    Q_OBJECT

    public:
        WBDocumentToolsPalette(QWidget *parent = 0);
        virtual ~WBDocumentToolsPalette();
        bool isEmpty() { return actions().count() == 0; }

};

#endif /* WBDOCUMENTTOOLSPALLETTE_H_ */
