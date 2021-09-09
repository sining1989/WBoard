#ifndef WBMETADATADCSUBSETADAPTOR_H_
#define WBMETADATADCSUBSETADAPTOR_H_

#include <QtWidgets>

class WBDocumentProxy;

class WBMetadataDcSubsetAdaptor
{
    public:
        WBMetadataDcSubsetAdaptor();
        virtual ~WBMetadataDcSubsetAdaptor();

        static void persist(WBDocumentProxy* proxy);
        static QMap<QString, QVariant> load(QString pPath);

        static const QString nsRdf;
        static const QString nsDc;
        static const QString metadataFilename;

        static const QString rdfIdentifierDomain;

};

#endif /* WBMETADATADCSUBSETADAPTOR_H_ */
