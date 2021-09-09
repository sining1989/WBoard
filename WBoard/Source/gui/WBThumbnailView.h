#ifndef WBTHUMBNAILVIEW_H_
#define WBTHUMBNAILVIEW_H_

#include <QGraphicsView>
#include <QLabel>
#include <QHBoxLayout>
#include <QDebug>

class WBGraphicsScene;

class WBThumbnailView : public QGraphicsView
{
    Q_OBJECT

public:
    WBThumbnailView(WBGraphicsScene *scene, QWidget* parent =0);
    virtual ~WBThumbnailView()
    {

    }

private:
    QHBoxLayout* mHBoxLayout;

};

#endif /* WBTHUMBNAILVIEW_H_ */
