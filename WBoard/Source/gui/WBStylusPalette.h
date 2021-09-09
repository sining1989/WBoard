#ifndef WBSTYLUSPALLETTE_H_
#define WBSTYLUSPALLETTE_H_

#include <QButtonGroup>

#include "WBActionPalette.h"

class WBStylusPalette : public WBActionPalette
{
    Q_OBJECT

public:
    WBStylusPalette(QWidget *parent = 0, Qt::Orientation orient = Qt::Vertical);
    virtual ~WBStylusPalette();

    void initPosition();
		
private slots:
    void stylusToolDoubleClicked();

private:
    int mLastSelectedId;

signals:
    void stylusToolDoubleClicked(int tool);
};

#endif /* WBSTYLUSPALLETTE_H_ */
