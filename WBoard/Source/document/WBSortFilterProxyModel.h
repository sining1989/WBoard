#ifndef WBSORTFILTERPROXYMODEL_H
#define WBSORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include "core/WBPersistenceManager.h"

class WBSortFilterProxyModel : public QSortFilterProxyModel
{
public:
    WBSortFilterProxyModel();

    bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
};

#endif // WBSORTFILTERPROXYMODEL_H
