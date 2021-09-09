#include "WBMetadataDcSubsetAdaptor.h"

#include <QtWidgets>
#include <QtXml>
#include <QDesktopWidget>

#include "core/WBSettings.h"
#include "core/WBApplication.h"
#include "board/WBBoardController.h"

#include "document/WBDocumentProxy.h"

#include "core/memcheck.h"

const QString WBMetadataDcSubsetAdaptor::nsRdf = "http://www.w3.org/1999/02/22-rdf-syntax-ns#";
const QString WBMetadataDcSubsetAdaptor::nsDc = "http://purl.org/dc/elements/1.1/";
const QString WBMetadataDcSubsetAdaptor::metadataFilename = "metadata.rdf";


WBMetadataDcSubsetAdaptor::WBMetadataDcSubsetAdaptor()
{

}


WBMetadataDcSubsetAdaptor::~WBMetadataDcSubsetAdaptor()
{
    // NOOP
}


void WBMetadataDcSubsetAdaptor::persist(WBDocumentProxy* proxy)
{
    if(!QDir(proxy->persistencePath()).exists()){
        //In this case the a document is an empty document so we do not persist it
        return;
    }
    QString fileName = proxy->persistencePath() + "/" + metadataFilename;
    qWarning() << "Persisting document; path is" << fileName;
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        qCritical() << "cannot open " << fileName << " for writing ...";
        qCritical() << "error : "  << file.errorString();
        return;
    }

    QXmlStreamWriter xmlWriter(&file);
    xmlWriter.setAutoFormatting(true);

    xmlWriter.writeStartDocument();
    xmlWriter.writeDefaultNamespace(nsRdf);
    xmlWriter.writeNamespace(nsDc, "dc");
    xmlWriter.writeNamespace(WBSettings::uniboardDocumentNamespaceUri, "ub");

    xmlWriter.writeStartElement("RDF");

    xmlWriter.writeStartElement("Description");
    xmlWriter.writeAttribute("about", proxy->metaData(WBSettings::documentIdentifer).toString());

    xmlWriter.writeTextElement(nsDc, "title", proxy->metaData(WBSettings::documentName).toString());
    xmlWriter.writeTextElement(nsDc, "type", proxy->metaData(WBSettings::documentGroupName).toString());
    xmlWriter.writeTextElement(nsDc, "date", proxy->metaData(WBSettings::documentDate).toString());
    xmlWriter.writeTextElement(nsDc, "format", "image/svg+xml");

    xmlWriter.writeTextElement(nsDc, "identifier", proxy->metaData(WBSettings::documentIdentifer).toString());
    xmlWriter.writeTextElement(WBSettings::uniboardDocumentNamespaceUri, "version", WBSettings::currentFileVersion);
    QString width = QString::number(proxy->defaultDocumentSize().width());
    QString height = QString::number(proxy->defaultDocumentSize().height());
    xmlWriter.writeTextElement(WBSettings::uniboardDocumentNamespaceUri, "size", QString("%1x%2").arg(width).arg(height));

    xmlWriter.writeTextElement(WBSettings::uniboardDocumentNamespaceUri, "updated-at", WBStringUtils::toUtcIsoDateTime(QDateTime::currentDateTimeUtc()));

    xmlWriter.writeTextElement(WBSettings::uniboardDocumentNamespaceUri, "page-count", QString::number(proxy->pageCount()));

    xmlWriter.writeEndElement(); 
    xmlWriter.writeEndElement();

    xmlWriter.writeEndDocument();

    file.flush();
    file.close();
}


QMap<QString, QVariant> WBMetadataDcSubsetAdaptor::load(QString pPath)
{

    QMap<QString, QVariant> metadata;

    QString fileName = pPath + "/" + metadataFilename;

    QFile file(fileName);

    bool sizeFound = false;
    bool updatedAtFound = false;
    QString date;

    if (file.exists())
    {
        if (!file.open(QIODevice::ReadOnly))
        {
            qWarning() << "Cannot open file " << fileName << " for reading ...";
            return metadata;
        }

        QXmlStreamReader xml(&file);

        while (!xml.atEnd())
        {
            xml.readNext();

            if (xml.isStartElement())
            {
                QString docVersion = "4.1"; // untagged doc version 4.1

                if (xml.name() == "title")
                {
                    metadata.insert(WBSettings::documentName, xml.readElementText());
                }
                else if (xml.name() == "type")
                {
                    metadata.insert(WBSettings::documentGroupName, xml.readElementText());
                }
                else if (xml.name() == "date")
                {
                    date = xml.readElementText();
                }
                else if (xml.name() == "identifier")
                {
                        metadata.insert(WBSettings::documentIdentifer, xml.readElementText());
                }
                else if (xml.name() == "version"
                        && xml.namespaceUri() == WBSettings::uniboardDocumentNamespaceUri)
                {
                        docVersion = xml.readElementText();
                }
                else if (xml.name() == "size"
                        && xml.namespaceUri() == WBSettings::uniboardDocumentNamespaceUri)
                {
                    QString size = xml.readElementText();
                    QStringList sizeParts = size.split("x");
                    bool ok = false;
                    int width, height;
                    if (sizeParts.count() >= 2)
                    {
                        bool widthOK, heightOK;
                        width = sizeParts.at(0).toInt(&widthOK);
                        height = sizeParts.at(1).toInt(&heightOK);
                        ok = widthOK && heightOK;

                        QSize docSize(width, height);

                        if (width == 1024 && height == 768) // move from 1024/768 to 1280/960
                        {
                            docSize = WBSettings::settings()->pageSize->get().toSize();
                        }

                        metadata.insert(WBSettings::documentSize, QVariant(docSize));
                    }
                    if (!ok)
                    {
                        qWarning() << "Invalid document size:" << size;
                    }

                    sizeFound = true;

                }
                else if (xml.name() == "updated-at"
                        && xml.namespaceUri() == WBSettings::uniboardDocumentNamespaceUri)
                {
                    metadata.insert(WBSettings::documentUpdatedAt, xml.readElementText());
                    updatedAtFound = true;
                }
                else if (xml.name() == "page-count"
                        && xml.namespaceUri() == WBSettings::uniboardDocumentNamespaceUri)
                {
                    metadata.insert(WBSettings::documentPageCount, xml.readElementText());
                }
                metadata.insert(WBSettings::documentVersion, docVersion);
            }

            if (xml.hasError())
            {
                qWarning() << "error parsing metadata.rdf file " << xml.errorString();
            }
        }

        file.close();
    }

    if (!sizeFound)
    {
        QDesktopWidget* dw = qApp->desktop();
        int controlScreenIndex = dw->primaryScreen();

        QSize docSize = dw->screenGeometry(controlScreenIndex).size();
        docSize.setHeight(docSize.height() - 70); // 70 = toolbar height

        qWarning() << "Document size not found, using default view size" << docSize;

        metadata.insert(WBSettings::documentSize, QVariant(docSize));
    }

    // this is necessary to update the old files date
    QString dateString = metadata.value(WBSettings::documentDate).toString();
    if(dateString.length() < 10){
        metadata.remove(WBSettings::documentDate);
        metadata.insert(WBSettings::documentDate,dateString+"T00:00:00Z");
    }

    if (!updatedAtFound) {
        metadata.insert(WBSettings::documentUpdatedAt, dateString);
    }

    metadata.insert(WBSettings::documentDate, QVariant(date));


    return metadata;
}

