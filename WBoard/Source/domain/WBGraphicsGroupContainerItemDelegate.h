#ifndef WBGRAPHICSGROUPCONTAINERITEMDELEGATE_H
#define WBGRAPHICSGROUPCONTAINERITEMDELEGATE_H

#include "domain/WBGraphicsItemDelegate.h"

class WBGraphicsGroupContainerItem;

class WBGraphicsGroupContainerItemDelegate : public WBGraphicsItemDelegate
{
Q_OBJECT

public:
    WBGraphicsGroupContainerItemDelegate(QGraphicsItem* pDelegated, QObject * parent = 0);
    WBGraphicsGroupContainerItem *delegated();

protected:
    virtual void decorateMenu(QMenu *menu);
    virtual void buildButtons();
    virtual void freeButtons();

    virtual bool mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual bool mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual bool mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private slots:
    void destroyGroup();

private:
    DelegateButton *mDestroyGroupButton;
};

#endif // WBGRAPHICSGROUPCONTAINERITEMDELEGATE_H
