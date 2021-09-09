#ifndef WBGRAPHICSWIDGETITEMDELEGATE_H_
#define WBGRAPHICSWIDGETITEMDELEGATE_H_

#include <QtWidgets>

#include "WBGraphicsItemDelegate.h"
#include "WBGraphicsWidgetItem.h"

class WBGraphicsWidgetItemDelegate : public WBGraphicsItemDelegate
{
    Q_OBJECT

    public:
        WBGraphicsWidgetItemDelegate(WBGraphicsWidgetItem* pDelegated, int widgetType = 0);
        virtual ~WBGraphicsWidgetItemDelegate();

    protected:
        virtual void decorateMenu(QMenu* menu);
        virtual void updateMenuActionState();
        virtual void remove(bool canundo);

    private slots:
        void freeze(bool frozeon);
        void pin();

    private:
        int mWidgetType;

        WBGraphicsWidgetItem* delegated();

        QAction* freezeAction;
        QAction* setAsToolAction;
};

#endif /* WBGRAPHICSWIDGETITEMDELEGATE_H_ */
