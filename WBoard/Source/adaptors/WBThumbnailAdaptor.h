#ifndef WBTHUMBNAILADAPTOR_H
#define WBTHUMBNAILADAPTOR_H

#include <QtCore>

class WBDocument;
class WBDocumentProxy;
class WBGraphicsScene;

class WBThumbnailAdaptor
{
    Q_DECLARE_TR_FUNCTIONS(WBThumbnailAdaptor)

public:
    static QUrl thumbnailUrl(WBDocumentProxy* proxy, int pageIndex);

    static void persistScene(WBDocumentProxy* proxy, WBGraphicsScene* pScene, int pageIndex, bool overrideModified = false);

    static const QPixmap* get(WBDocumentProxy* proxy, int index);
    static void load(WBDocumentProxy* proxy, QList<const QPixmap*>& list);

private:
    static void generateMissingThumbnails(WBDocumentProxy* proxy);

    WBThumbnailAdaptor() {}
};

#endif // WBTHUMBNAILADAPTOR_H
