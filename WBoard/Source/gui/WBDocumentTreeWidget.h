#ifndef WBDOCUMENTTREEWIDGET_H_
#define WBDOCUMENTTREEWIDGET_H_

#include <QtWidgets>
#include <QTreeWidget>

class WBDocumentProxy;
class WBDocumentProxyTreeItem;
class WBDocumentGroupTreeItem;

class WBDocumentTreeWidget : public QTreeWidget
{
    Q_OBJECT

    public:
        WBDocumentTreeWidget(QWidget *parent = 0);
        virtual ~WBDocumentTreeWidget();

    protected:
        virtual void dragEnterEvent(QDragEnterEvent *event);
        virtual void dragLeaveEvent(QDragLeaveEvent *event);
        virtual void dropEvent(QDropEvent *event);
        virtual void mousePressEvent(QMouseEvent *event);
        virtual void dragMoveEvent(QDragMoveEvent *event);
        virtual void focusInEvent(QFocusEvent *event);
        virtual Qt::DropActions supportedDropActions() const;

    private slots:
        void documentUpdated(WBDocumentProxy *pDocument);

        void itemChangedValidation(QTreeWidgetItem * item, int column);
        void autoScroll();

    private:
        bool moveDocument(WBDocumentProxyTreeItem* document, WBDocumentGroupTreeItem* destinationFolder);
        WBDocumentProxyTreeItem *mSelectedProxyTi;
        QTreeWidgetItem *mDropTargetProxyTi;
        QBrush mBackground;
        QTimer* mScrollTimer;
        int mScrollMagnitude;
};


class WBDocumentProxyTreeItem : public QTreeWidgetItem
{
    public:
        WBDocumentProxyTreeItem(QTreeWidgetItem * parent, WBDocumentProxy* proxy, bool isEditable = true);

        QPointer<WBDocumentProxy> proxy() const
        {
            return mProxy;
        }

        bool isInTrash();

        QPointer<WBDocumentProxy> mProxy;
};

class WBDocumentGroupTreeItem : public QTreeWidgetItem
{
    public:
        WBDocumentGroupTreeItem(QTreeWidgetItem *parent, bool isEditable = true);
        virtual ~WBDocumentGroupTreeItem();

        void setGroupName(const QString &groupName);

        QString groupName() const;

        bool isTrashFolder() const;
        bool isDefaultFolder() const;
};

#endif /* WBDOCUMENTTREEWIDGET_H_ */
