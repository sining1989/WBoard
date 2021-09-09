#ifndef WBRESIZABLEGRAPHICSITEM_H_
#define WBRESIZABLEGRAPHICSITEM_H_

#include <QtWidgets>

class WBResizableGraphicsItem
{
    public:
        WBResizableGraphicsItem();
        virtual ~WBResizableGraphicsItem();

        virtual void resize(const QSizeF& pSize);
        virtual void resize(qreal w, qreal h) = 0;

        virtual QSizeF size() const = 0;

};

#endif /* WBRESIZABLEGRAPHICSITEM_H_ */
