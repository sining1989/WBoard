#include "WBImportAdaptor.h"

#include "core/WBApplication.h"
#include "core/WBPersistenceManager.h"

#include "document/WBDocumentProxy.h"

#include "core/memcheck.h"

WBImportAdaptor::WBImportAdaptor(bool _documentBased, QObject *parent)
    :QObject(parent),
    documentBased(_documentBased)
{
    // NOOP
}

WBImportAdaptor::~WBImportAdaptor()
{
    // NOOP
}

WBPageBasedImportAdaptor::WBPageBasedImportAdaptor(QObject *parent)
    :WBImportAdaptor(false, parent)
{
    // NOOP
}

WBDocumentBasedImportAdaptor::WBDocumentBasedImportAdaptor(QObject *parent)
    :WBImportAdaptor(true, parent)
{
    // NOOP
}
