#include <QtWidgets>
#include <QtSvg>

#include "domain/WBGraphicsScene.h"
#include "domain/WBGraphicsDelegateFrame.h"
#include "WBGraphicsCurtainItemDelegate.h"
#include "WBGraphicsCurtainItem.h"

#include "core/memcheck.h"

WBGraphicsCurtainItemDelegate::WBGraphicsCurtainItemDelegate(WBGraphicsCurtainItem* pDelegated, QObject * parent)
    : WBGraphicsItemDelegate(pDelegated, parent, GF_MENU_SPECIFIED | GF_DUPLICATION_ENABLED)
{
    //NOOP
}


WBGraphicsCurtainItemDelegate::~WBGraphicsCurtainItemDelegate()
{
    //NOOP
}

void WBGraphicsCurtainItemDelegate::init()
{
    if(!mFrame){
        createControls();
        mFrame->hide();
        mZOrderUpButton->hide();
        mZOrderDownButton->hide();
    }
}


bool WBGraphicsCurtainItemDelegate::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);

    startUndoStep();

    //if (!mDelegated->isSelected())
    //{
    //    mDelegated->setSelected(true);
    //    positionHandles();

    //    return true;
    //}
    //else
    {
        return false;
    }

}


QVariant WBGraphicsCurtainItemDelegate::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
    if (change == QGraphicsItem::ItemVisibleHasChanged)
    {
        //WBGraphicsScene* ubScene = qobject_cast<WBGraphicsScene*>(mDelegated->scene());
        //if(ubScene)
        //    ubScene->setModified(true);
    }

    return WBGraphicsItemDelegate::itemChange(change, value);
}

void WBGraphicsCurtainItemDelegate::positionHandles()
{
    WBGraphicsItemDelegate::positionHandles();
    if(mZOrderUpButton)
        mZOrderUpButton->hide();
    if(mZOrderDownButton)
        mZOrderDownButton->hide();
}

void WBGraphicsCurtainItemDelegate::remove(bool checked, bool canUndo)
{
    Q_UNUSED(checked);
    Q_UNUSED(canUndo);

    WBGraphicsCurtainItem *curtain = dynamic_cast<WBGraphicsCurtainItem*>(mDelegated);

    if (curtain)
    {
        curtain->setVisible(false);
        curtain->triggerRemovedSignal();
    }
}

