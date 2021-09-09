#ifndef XPDFRENDERER_H
#define XPDFRENDERER_H
#include <QImage>
#include "PDFRenderer.h"
//#include <splash/SplashBitmap.h>

#include "globals/WBGlobals.h"

//THIRD_PARTY_WARNINGS_DISABLE
//#include <xpdf/Object.h>
//#include <xpdf/GlobalParams.h>
//#include <xpdf/SplashOutputDev.h>
//#include <xpdf/PDFDoc.h>
//THIRD_PARTY_WARNINGS_ENABLE

class PDFDoc;

class XPDFRenderer : public PDFRenderer
{
    Q_OBJECT

    public:
        XPDFRenderer(const QString &filename, bool importingFile = false);
        virtual ~XPDFRenderer();

        bool isValid() const;

        virtual int pageCount() const;

        virtual QSizeF pageSizeF(int pageNumber) const;

        virtual int pageRotation(int pageNumber) const;

        virtual QString title() const;

    public slots:
        void render(QPainter *p, int pageNumber, const QRectF &bounds = QRectF());

    private:
        void init();
        //QImage* createPDFImage(int pageNumber, qreal xscale = 0.5, qreal yscale = 0.5, const QRectF &bounds = QRectF());

        PDFDoc *mDocument;
        static QAtomicInt sInstancesCount;
        qreal mSliceX;
        qreal mSliceY;

        //SplashBitmap* mpSplashBitmap;
        //SplashOutputDev* mSplash;
};

#endif // XPDFRENDERER_H
