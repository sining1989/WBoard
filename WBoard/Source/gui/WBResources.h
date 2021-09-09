#ifndef WBRESOURCES_H_
#define WBRESOURCES_H_

#include <QtWidgets>

class WBResources : public QObject
{
    Q_OBJECT

public:
	static WBResources* resources();
	QStringList customFontList() { return mCustomFontList; }

private:
    WBResources(QObject* pParent = 0);
    virtual ~WBResources();

    void init();

    static WBResources* sSingleton;
    void buildFontList();
    QStringList mCustomFontList;

public:
    QCursor penCursor;
    QCursor eraserCursor;
    QCursor markerCursor;
    QCursor pointerCursor;
    QCursor handCursor;
    QCursor zoomInCursor;
    QCursor zoomOutCursor;
    QCursor arrowCursor;
    QCursor playCursor;
    QCursor textCursor;
    QCursor rotateCursor;
    QCursor drawLineRulerCursor;
};

#endif /* WBRESOURCES_H_ */
