#ifndef WBCOLORPICKER_H_
#define WBCOLORPICKER_H_

#include <QtWidgets>
#include <QFrame>

class WBColorPicker : public QFrame
{
    Q_OBJECT

    public:
        WBColorPicker(QWidget* parent);
        WBColorPicker(QWidget* parent, const QList<QColor>& colors, int pSelectedColorIndex = 0);
        virtual ~WBColorPicker();
        QList<QColor> getColors() const
        {
            return mColors;
        }

        void setColors(const QList<QColor>& pColors)
        {
            mColors = pColors;
            repaint();
        }

        int selectedColorIndex() const
        {
            return mSelectedColorIndex;
        }

        void setSelectedColorIndex(int pSelectedColorIndex)
        {
            mSelectedColorIndex = pSelectedColorIndex;
            repaint();
        }

    signals:
        void colorSelected(const QColor& color);

    protected:
        virtual void paintEvent ( QPaintEvent * event );
        virtual void mousePressEvent ( QMouseEvent * event );

    private:
        QList<QColor> mColors;
        int mSelectedColorIndex;
};

#endif /* WBCOLORPICKER_H_ */
