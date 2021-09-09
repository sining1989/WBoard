#ifndef WBGRAPHICSTEXTITEMDELEGATE_H_
#define WBGRAPHICSTEXTITEMDELEGATE_H_

#include <QtWidgets>

#include <QtSvg>

#include "core/WB.h"
#include "WBGraphicsItemDelegate.h"

class WBGraphicsTextItem;

class AlignTextButton : public DelegateButton
{
    Q_OBJECT

public:
    static const int MAX_KIND = 3;
    enum kind_t{
        k_left = 0
        , k_center
        , k_right
        , k_mixed
    };

    AlignTextButton(const QString & fileName, QGraphicsItem* pDelegated, QGraphicsItem * parent = 0, Qt::WindowFrameSection section = Qt::TopLeftSection);
    virtual ~AlignTextButton();

    void setKind(int pKind);
    int kind() {return mKind;}

    void setNextKind();
    int nextKind() const;

    void setMixedButtonVisible(bool v = true) {mHideMixed = !v;}
    bool isMixedButtonVisible() {return !mHideMixed;}

private:
    QSvgRenderer *rndFromKind(int pknd)
    {
        switch (pknd) {
        case k_left:
            return lft;
            break;
        case k_center:
            return cntr;
            break;
        case k_right:
            return rght;
            break;
        case k_mixed:
            return mxd;
            break;
        }

        return 0;
    }

    QSvgRenderer *curRnd() {return rndFromKind(mKind);}

    QPointer<QSvgRenderer> lft;
    QPointer<QSvgRenderer> cntr;
    QPointer<QSvgRenderer> rght;
    QPointer<QSvgRenderer> mxd;

    int mKind;
    bool mHideMixed;
};

class WBGraphicsTextItemDelegate : public WBGraphicsItemDelegate
{
    Q_OBJECT

    enum textChangeMode
    {
        changeSize = 0,
        scaleSize
    };

    public:
        WBGraphicsTextItemDelegate(WBGraphicsTextItem* pDelegated, QObject * parent = 0);
        virtual ~WBGraphicsTextItemDelegate();
        bool isEditable();
        void scaleTextSize(qreal multiplyer);
        void recolor();
        virtual QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value);
        virtual void createControls();
        qreal titleBarWidth();

    public slots:
        void contentsChanged();
        virtual void setEditable(bool);
        virtual void remove(bool canUndo);

    protected:
        virtual void decorateMenu(QMenu *menu);
        virtual void updateMenuActionState();

        virtual void freeButtons();

        virtual bool mousePressEvent(QGraphicsSceneMouseEvent *event);
        virtual bool mouseMoveEvent(QGraphicsSceneMouseEvent *event);
        virtual bool mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

        virtual bool keyPressEvent(QKeyEvent *event);
        virtual bool keyReleaseEvent(QKeyEvent *event);

    private:
        WBGraphicsTextItem* delegated();

        DelegateButton* mFontButton;
        DelegateButton* mColorButton;
        DelegateButton* mDecreaseSizeButton;
        DelegateButton* mIncreaseSizeButton;
        DelegateButton* mAlignButton;

        int mLastFontPixelSize;

        static const int sMinPixelSize;
        static const int sMinPointSize;

    private:
        void customize(QFontDialog &fontDialog);
        void ChangeTextSize(qreal factor, textChangeMode changeMode);
        void updateAlighButtonState();
        bool oneBlockSelection();
        void saveTextCursorFormats();
        void restoreTextCursorFormats();

        QFont createDefaultFont();
        QAction *mEditableAction;
        struct selectionData_t {
            selectionData_t()
                : mButtonIsPressed(false)
            {}
            bool mButtonIsPressed;
            int position;
            int anchor;
            QString html;
            QTextDocumentFragment selection;
            QList<QTextBlockFormat> fmts;

        } mSelectionData;

    private slots:
        void pickFont();
        void pickColor();

        void decreaseSize();
        void increaseSize();

        void alignButtonProcess();
        void onCursorPositionChanged(const QTextCursor& cursor);
        void onModificationChanged(bool ch);
        void onContentChanged();

	private:
      const int delta;
};

#endif /* WBGRAPHICSTEXTITEMDELEGATE_H_ */
