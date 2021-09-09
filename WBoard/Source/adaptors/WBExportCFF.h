#ifndef WBExportCFF_H_
#define WBExportCFF_H_

#include <QtCore>

#include "WBExportAdaptor.h"

#include "frameworks/WBFileSystemUtils.h"

class WBDocumentProxy;

class WBExportCFF : public WBExportAdaptor
{
    Q_OBJECT

public:
    WBExportCFF(QObject *parent = 0);
    virtual ~WBExportCFF();

    virtual QString exportName();
    virtual QString exportExtention();
    virtual void persist(WBDocumentProxy* pDocument);
    virtual bool associatedActionactionAvailableFor(const QModelIndex &selectedIndex);
};

#endif /* WBExportCFF_H_ */
