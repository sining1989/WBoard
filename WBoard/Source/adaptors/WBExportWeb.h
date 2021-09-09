#ifndef WBEXPORTWEB_H_
#define WBEXPORTWEB_H_

#include <QtCore>

#include "WBExportAdaptor.h"

class WBDocumentProxy;

class WBExportWeb : public WBExportAdaptor
{
    Q_OBJECT;

    public:
        WBExportWeb(QObject *parent = 0);
        virtual ~WBExportWeb();

        virtual QString exportName();
        virtual void persist(WBDocumentProxy* pDocument);

};

#endif /* WBEXPORTWEB_H_ */
