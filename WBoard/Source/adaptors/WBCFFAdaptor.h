#ifndef WBCFFADAPTOR_H
#define WBCFFADAPTOR_H

#include "WBCFFAdaptor_global.h"

#include <QtCore>

class QTransform;
class QDomDocument;
class QDomElement;
class QDomNode;
class QuaZipFile;

class WBCFFAdaptor {
    class WBToCFFConverter;

public:
    WBCFFAdaptor();
    ~WBCFFAdaptor();

    bool convertWBZToIWB(const QString &from, const QString &to);
    bool deleteDir(const QString& pDirPath) const;
    QList<QString> getConversionMessages();

private:
    QString uncompressZip(const QString &zipFile);
    bool compressZip(const QString &source, const QString &destination);
    bool compressDir(const QString &dirName, const QString &parentDir, QuaZipFile *outZip);
    bool compressFile(const QString &fileName, const QString &parentDir, QuaZipFile *outZip);

    QString createNewTmpDir();
    bool freeDir(const QString &dir);
    void freeTmpDirs();

private:
    QStringList tmpDirs;
    QList<QString> mConversionMessages;

private:
    class WBToCFFConverter {
       static const int DEFAULT_LAYER = -100000;

    public:
        WBToCFFConverter(const QString &source, const QString &destination);
        ~WBToCFFConverter();
        bool isValid() const;
        QString lastErrStr() const {return errorStr;}
        bool parse();
        QList<QString> getMessages() {return mExportErrorList;}

    private:
        void addLastExportError(QString error) {mExportErrorList.append(error);}

        void fillNamespaces();

        bool parseMetadata();
        bool parseContent();
        QDomElement parsePageset(const QStringList &pageFileNames);
        QDomElement parsePage(const QString &pageFileName);
        QDomElement parseSvgPageSection(const QDomElement &element);
        void writeQDomElementToXML(const QDomNode &node);
        bool writeExtendedIwbSection();
        QDomElement parseGroupsPageSection(const QDomElement &groupRoot);

        bool createBackground(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList);
        QString createBackgroundImage(const QDomElement &element, QSize size);
        bool createPngFromSvg(QString &svgPath, QString &dstPath,  QTransform transformation, QSize size = QSize());

        bool parseSVGGGroup(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList);
        bool parseWBZImage(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList);
        bool parseWBZVideo(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList);
        bool parseWBZAudio(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList);
        bool parseForeignObject(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList);
        bool parseWBZText(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList);

        bool parseWBZPolygon(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList);
        bool parseWBZPolyline(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList);
        bool parseWBZLine(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList);       
        void addSVGElementToResultModel(const QDomElement &element, QMultiMap<int, QDomElement> &dstList, int layer = DEFAULT_LAYER);
        void addIWBElementToResultModel(const QDomElement &element);

        qreal getAngleFromTransform(const QTransform &tr);
        QString getDstContentFolderName(const QString &elementType);
        QString getSrcContentFolderName(QString href);
        QString getFileNameFromPath(QString sPath);
        QString getExtentionFromFileName(const QString &filename);
        QString convertExtention(const QString &ext);
        QString getElementTypeFromUBZ(const QDomElement &element);

        int getElementLayer(const QDomElement &element);

        bool itIsSupportedFormat(const QString &format) const;
        bool itIsFormatToConvert(const QString &format) const;
        bool itIsSVGElementAttribute(const QString ItemType, const QString &AttrName);
        bool itIsIWBAttribute(const QString &attribute) const;
        bool itIsWBZAttributeToConvert(const QString &attribute) const;

        bool ibwAddLine(int x1, int y1, int x2, int y2, QString color=QString(), int width=1, bool isBackground=false);

        QTransform getTransformFromWBZ(const QDomElement &ubzElement);
        void setGeometryFromWBZ(const QDomElement &ubzElement, QDomElement &iwbElement);
        void setCoordinatesFromWBZ(const QDomElement &ubzElement, QDomElement &iwbElement);
        bool setContentFromWBZ(const QDomElement &ubzElement, QDomElement &svgElement);
        void setCFFTextFromHTMLTextNode(const QDomElement htmlTextNode, QDomElement &iwbElement);
        QString ubzAttrNameToCFFAttrName(QString cffAttrName);
        QString ubzAttrValueToCFFAttrName(QString cffAttrValue);

        bool setCFFAttribute(const QString &attributeName, const QString &attributeValue, const QDomElement &ubzElement, QDomElement &iwbElement,  QDomElement &svgElement);
        bool setCommonAttributesFromUBZ(const QDomElement &ubzElement, QDomElement &iwbElement,  QDomElement &svgElement);
        void setViewBox(QRect viewbox);

        QDomNode findTextNode(const QDomNode &node);
        QDomNode findNodeByTagName(const QDomNode &node, QString tagName);

        QSize getSVGDimentions(const QString &element);

        inline QRect getViewboxRect(const QString &element) const;
        inline QString rectToIWBAttr(const QRect &rect) const;
        inline QString digitFileFormat(int num) const;
        inline bool strToBool(const QString &in) const {return in == "true";}
        QString contentIWBFileName() const;

    private:
        QList<QString> mExportErrorList;
        QMap<QString, QString> iwbSVGItemsAttributes;
        QDomDocument *mDataModel;
        QXmlStreamWriter *mIWBContentWriter;
        QSize mSVGSize;
        QRect mViewbox;
        QString sourcePath;
        QString destinationPath;
        QDomDocument *mDocumentToWrite;
        QMultiMap<int, QDomElement> mSvgElements;
        QList<QDomElement> mExtendedElements;
        mutable QString errorStr;

    public:
        operator bool() const {return isValid();}
    };
};

#endif // WBCFFADAPTOR_H
