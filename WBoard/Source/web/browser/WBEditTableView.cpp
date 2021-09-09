#include "WBEditTableView.h"
//#include <QtWidgets/QKeyEvent>
#include <QtWidgets/qkeyeventtransition.h>

#include "core/memcheck.h"

WBEditTableView::WBEditTableView(QWidget *parent)
    : QTableView(parent)
{
}

void WBEditTableView::keyPressEvent(QKeyEvent *event)
{
    if ((event->key() == Qt::Key_Delete
        || event->key() == Qt::Key_Backspace)
        && model())
    {
        removeOne();
    } else {
        QAbstractItemView::keyPressEvent(event);
    }
}

void WBEditTableView::removeOne()
{
    if (!model() || !selectionModel())
        return;
    int row = currentIndex().row();
    model()->removeRow(row, rootIndex());
    QModelIndex idx = model()->index(row, 0, rootIndex());
    if (!idx.isValid())
        idx = model()->index(row - 1, 0, rootIndex());
    selectionModel()->select(idx, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
}

void WBEditTableView::removeAll()
{
    if (model())
        model()->removeRows(0, model()->rowCount(rootIndex()), rootIndex());
}

