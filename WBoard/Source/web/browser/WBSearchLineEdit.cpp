#include "WBSearchLineEdit.h"

#include <QtWidgets>
#include <QMenu>

#include "core/memcheck.h"

WBClearButton::WBClearButton(QWidget *parent)
  : QAbstractButton(parent)
{
    setCursor(Qt::ArrowCursor);
    setToolTip(tr("Clear"));
    setVisible(false);
    setFocusPolicy(Qt::NoFocus);
}

void WBClearButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    int height = this->height();

    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setBrush(isDown()
                     ? palette().color(QPalette::Dark)
                     : palette().color(QPalette::Mid));
    painter.setPen(painter.brush().color());
    int size = width();
    int offset = size / 5;
    int radius = size - offset * 2;
    painter.drawEllipse(offset, offset, radius, radius);

    painter.setPen(palette().color(QPalette::Base));
    int border = offset * 2;
    painter.drawLine(border, border, width() - border, height - border);
    painter.drawLine(border, height - border, width() - border, border);
}

void WBClearButton::textChanged(const QString &text)
{
    setVisible(!text.isEmpty());
}

/*
    Search icon on the left hand side of the search widget
    When a menu is set a down arrow appears
 */
class WBSearchButton : public QAbstractButton
{
    public:
        WBSearchButton(QWidget *parent = 0);
        void paintEvent(QPaintEvent *event);
        QMenu *m_menu;

    protected:
        void mousePressEvent(QMouseEvent *event);
};

WBSearchButton::WBSearchButton(QWidget *parent)
  : QAbstractButton(parent),
    m_menu(0)
{
    setObjectName(QLatin1String("SearchButton"));
    setCursor(Qt::ArrowCursor);
    setFocusPolicy(Qt::NoFocus);
}

void WBSearchButton::mousePressEvent(QMouseEvent *event)
{
    if (m_menu && event->button() == Qt::LeftButton)
    {
        QWidget *p = parentWidget();
        if (p)
        {
            QPoint r = p->mapToGlobal(QPoint(0, p->height()));
            m_menu->exec(QPoint(r.x() + height() / 2, r.y()));
        }
        event->accept();
    }
    QAbstractButton::mousePressEvent(event);
}

void WBSearchButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainterPath myPath;

    int radius = (height() / 5) * 2;
    QRect circle(height() / 3 - 1, height() / 4, radius, radius);
    myPath.addEllipse(circle);

    myPath.arcMoveTo(circle, 300);
    QPointF c = myPath.currentPosition();
    int diff = height() / 7;
    myPath.lineTo(qMin(width() - 2, (int)c.x() + diff), c.y() + diff);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(Qt::darkGray, 2));
    painter.drawPath(myPath);

    if (m_menu)
    {
        QPainterPath dropPath;
        dropPath.arcMoveTo(circle, 320);
        QPointF c = dropPath.currentPosition();
        c = QPointF(c.x() + 3.5, c.y() + 0.5);
        dropPath.moveTo(c);
        dropPath.lineTo(c.x() + 4, c.y());
        dropPath.lineTo(c.x() + 2, c.y() + 2);
        dropPath.closeSubpath();
        painter.setPen(Qt::darkGray);
        painter.setBrush(Qt::darkGray);
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.drawPath(dropPath);
    }
    painter.end();
}

/*
    SearchLineEdit is an enhanced QLineEdit
    - A Search icon on the left with optional menu
    - When there is no text and doesn't have focus an "inactive text" is displayed
    - When there is text a clear button is displayed on the right hand side
 */
WBSearchLineEdit::WBSearchLineEdit(QWidget *parent)
    : WBExLineEdit(parent),
    mSearchButton(new WBSearchButton(this))
{
    connect(lineEdit(), SIGNAL(textChanged(const QString &)),
            this, SIGNAL(textChanged(const QString &)));
    setLeftWidget(mSearchButton);
    mInactiveText = tr("Search");

    setMinimumWidth(150);

    QSizePolicy policy = sizePolicy();
    setSizePolicy(QSizePolicy::Maximum, policy.verticalPolicy());
}

void WBSearchLineEdit::paintEvent(QPaintEvent *event)
{
    WBExLineEdit::paintEvent(event);

    if (lineEdit()->text().isEmpty() && !hasFocus() && !mInactiveText.isEmpty())
    {
        QStyleOptionFrameV2 panel;
        initStyleOption(&panel);
        QRect r = style()->subElementRect(QStyle::SE_LineEditContents, &panel, this);
        QFontMetrics fm = fontMetrics();
        int horizontalMargin = lineEdit()->x();
        QRect lineRect(horizontalMargin + r.x(), r.y() + (r.height() - fm.height() + 1) / 2,
                       r.width() - 2 * horizontalMargin, fm.height());
        QPainter painter(this);
        painter.setPen(palette().brush(QPalette::Disabled, QPalette::Text).color());
        painter.drawText(lineRect, Qt::AlignLeft|Qt::AlignVCenter, mInactiveText);
    }
}

void WBSearchLineEdit::resizeEvent(QResizeEvent *event)
{
    updateGeometries();
    WBExLineEdit::resizeEvent(event);
}

void WBSearchLineEdit::updateGeometries()
{
    int menuHeight = height();
    int menuWidth = menuHeight + 1;
    if (!mSearchButton->m_menu)
        menuWidth = (menuHeight / 5) * 4;
    mSearchButton->resize(QSize(menuWidth, menuHeight));
}

QString WBSearchLineEdit::inactiveText() const
{
    return mInactiveText;
}

void WBSearchLineEdit::setInactiveText(const QString &text)
{
    mInactiveText = text;
}

void WBSearchLineEdit::setMenu(QMenu *menu)
{
    if (mSearchButton->m_menu)
        mSearchButton->m_menu->deleteLater();
    mSearchButton->m_menu = menu;
    updateGeometries();
}

QMenu *WBSearchLineEdit::menu() const
{
    if (!mSearchButton->m_menu)
    {
        mSearchButton->m_menu = new QMenu(mSearchButton);
        if (isVisible())
            (const_cast<WBSearchLineEdit*>(this))->updateGeometries();
    }
    return mSearchButton->m_menu;
}


void WBSearchLineEdit::setVisible(bool pVisible)
{
    WBExLineEdit::setVisible(pVisible);
    mSearchButton->setVisible(pVisible);
}

