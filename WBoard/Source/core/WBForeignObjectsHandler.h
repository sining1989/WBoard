#ifndef WBFOREIGHNOBJECTSHANDLER_H
#define WBFOREIGHNOBJECTSHANDLER_H

#include <QList>
#include <QUrl>
#include <algorithm>

class WBForeighnObjectsHandlerPrivate;

class WBForeighnObjectsHandler
{
public:
    WBForeighnObjectsHandler();
    ~WBForeighnObjectsHandler();

    void cure(const QList<QUrl> &dirs);
    void cure(const QUrl &dir);

    void copyPage(const QUrl &fromDir, int fromIndex,
                  const QUrl &toDir, int toIndex);

private:
    WBForeighnObjectsHandlerPrivate *d;

    friend class WBForeighnObjectsHandlerPrivate;
};

#endif // WBFOREIGHNOBJECTSHANDLER_H
