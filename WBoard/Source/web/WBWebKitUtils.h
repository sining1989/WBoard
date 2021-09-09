#ifndef WBWEBKITUTILS_H_
#define WBWEBKITUTILS_H_

#include <QtWebEngine>
#include <QWebEnginePage>

class WBWebKitUtils
{
    public:
        WBWebKitUtils();
        virtual ~WBWebKitUtils();

        class HtmlObject
        {
            public:
                HtmlObject(const QString& pSource, int pWidth, int pHeight)
                    : source(pSource)
                    , width(pWidth)
                    , height(pHeight)
                {
         
                }

                QString source;
                int width;
                int height;

        };

        static QList<WBWebKitUtils::HtmlObject> objectsInFrame(QWebEnginePage* frame);

};

#endif /* WBWEBKITUTILS_H_ */
