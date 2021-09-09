#include "WBSortFilterProxyModel.h"
#include "WBDocumentController.h"

WBSortFilterProxyModel::WBSortFilterProxyModel():
    QSortFilterProxyModel()
{
    setDynamicSortFilter(false);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

bool WBSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    WBDocumentTreeModel *model = dynamic_cast<WBDocumentTreeModel*>(sourceModel());

    if(model){
        //if it's a top level folder
        //in other words : myDocuments, models and trash folder
        if(model->isToplevel(left) || model->isToplevel(right))
        {
            return false;
        }
    }

    return QSortFilterProxyModel::lessThan(left, right);
}
