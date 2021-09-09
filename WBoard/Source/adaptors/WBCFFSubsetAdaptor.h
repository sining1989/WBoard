#ifndef WBCFFSUBSETADAPTOR_H
#define WBCFFSUBSETADAPTOR_H

#include <QtXml>
#include <QString>
#include <QStack>
#include <QDomDocument>
#include <QHash>

class WBDocumentProxy;
class WBGraphicsScene;
class QSvgGenerator;
class WBGraphicsSvgItem;
class WBGraphicsPixmapItem;
class WBGraphicsItemDelegate;
class QTransform;
class QPainter;
class WBGraphicsItem;
class QGraphicsItem;
class QTextBlockFormat;
class QTextCharFormat;
class QTextCursor;
class WBGraphicsStrokesGroup;

class WBCFFSubsetAdaptor
{
public:
    WBCFFSubsetAdaptor();
    static bool ConvertCFFFileToWbz(QString &cffSourceFile, WBDocumentProxy* pDocument);

private:
    class WBCFFSubsetReader
    {
    public:
        WBCFFSubsetReader(WBDocumentProxy *proxy, QFile *content);
        ~WBCFFSubsetReader();

        WBDocumentProxy *mProxy;
        QString pwdContent;

        bool parse();

    private:
        QString mTempFilePath;
        WBGraphicsScene *mCurrentScene;
        QRectF mCurrentSceneRect;
        QString mIndent;
        QRectF mViewBox;
        QRectF mViewPort;
        qreal mVBTransFactor;
        QPointF mViewBoxCenter;
        QSize mSize;
        QPointF mShiftVector;
        bool mSvgGSectionIsOpened;
        WBGraphicsGroupContainerItem *mGSectionContainer;

    private:
        QDomDocument mDOMdoc;
        QDomNode mCurrentDOMElement;
        QHash<QString, WBGraphicsItem*> persistedItems;
        QMap<QString, QString> mRefToUuidMap;
        QDir mTmpFlashDir;

        void addItemToGSection(QGraphicsItem *item);
        bool hashElements();
        void addExtentionsToHash(QDomElement *parent, QDomElement *topGroup);

        void hashSvg(QDomNode *parent, QString prefix = "");
        void hashSiblingIwbElements(QDomElement *parent, QDomElement *topGroup = 0);

        inline void parseSvgSectionAttr(const QDomElement &);
        bool parseSvgPage(const QDomElement &parent);
        bool parseSvgPageset(const QDomElement &parent);
        bool parseSvgElement(const QDomElement &parent);
        bool parseIwbMeta(const QDomElement &element);
        bool parseSvg(const QDomElement &svgSection);

        inline bool parseGSection(const QDomElement &element);
        inline bool parseSvgSwitchSection(const QDomElement &element);
        inline bool parseSvgRect(const QDomElement &element);
        inline bool parseSvgEllipse(const QDomElement &element);
        inline bool parseSvgPolygon(const QDomElement &element);
        inline bool parseSvgPolyline(const QDomElement &element);
        inline bool parseSvgText(const QDomElement &element);
        inline bool parseSvgTextarea(const QDomElement &element);
        inline bool parseSvgImage(const QDomElement &element);
        inline bool parseSvgFlash(const QDomElement &element);
        inline bool parseSvgAudio(const QDomElement &element);
        inline bool parseSvgVideo(const QDomElement &element);
        inline WBGraphicsGroupContainerItem *parseIwbGroup(QDomElement &parent);
        inline bool parseIwbElement(QDomElement &element);
        inline void parseTSpan(const QDomElement &parent, QPainter &painter
                               , qreal &curX, qreal &curY, qreal &width, qreal &height, qreal &linespacing, QRectF &lastDrawnTextBoundingRect
                               , qreal &fontSize, QColor &fontColor, QString &fontFamily, QString &fontStretch, bool &italic
                               , int &fontWeight, int &textAlign, QTransform &fontTransform);
        inline void parseTSpan(const QDomElement &element, QTextCursor &cursor
                               , QTextBlockFormat &blockFormat, QTextCharFormat &charFormat);
        inline void hashSceneItem(const QDomElement &element, WBGraphicsItem *item);

        // to kill
        inline void parseTextAttributes(const QDomElement &element, qreal &fontSize, QColor &fontColor,
                                 QString &fontFamily, QString &fontStretch, bool &italic,
                                 int &fontWeight, int &textAlign, QTransform &fontTransform);
        inline void parseTextAttributes(const QDomElement &element, QFont &font, QColor);
        inline void readTextBlockAttr(const QDomElement &element, QTextBlockFormat &format);
        inline void readTextCharAttr(const QDomElement &element, QTextCharFormat &format);

        bool parseDoc();

        bool createNewScene();
        bool persistCurrentScene();
        bool persistScenes();

        void repositionSvgItem(QGraphicsItem *item, qreal width, qreal height,
                               qreal x, qreal y,
                               QTransform &transform);
        QColor colorFromString(const QString& clrString);
        QTransform transformFromString(const QString trString, QGraphicsItem *item = 0);
        bool getViewBoxDimenstions(const QString& viewBox);
        QSvgGenerator* createSvgGenerator(qreal width, qreal height);
        bool getTempFileName();
        inline bool strToBool(QString);
        bool createTempFlashPath();
    };
};

#endif // WBCFFSUBSETADAPTOR_H
