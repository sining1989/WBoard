#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QCheckBox>
#include <QPushButton>

#include "core/WBSettings.h"

#include "WBOpenSankoreImporterWidget.h"

WBOpenSankoreImporterWidget::WBOpenSankoreImporterWidget(QWidget *parent):
    WBFloatingPalette(Qt::TopRightCorner,parent)
{
    setBackgroundBrush(QBrush(Qt::white));

    setObjectName("WBOpenSankoreImporterWidget");
    setFixedSize(700,450);
    setStyleSheet("QWidget#WBOpenSankoreImporterWidget { background-color : red; }");

    QVBoxLayout* mLayout = new QVBoxLayout(this);
    mLayout->setContentsMargins(20,38,20,20);
    setLayout(mLayout);

    QLabel* title = new QLabel(this);
    title->setStyleSheet("font-size : 18px; font-weight : bold;");
    title->setText(tr("WBoard Documents Detected"));
    mLayout->addWidget(title);
    mLayout->addSpacing(20);

    QTextEdit* helpText = new QTextEdit(this);
    helpText->setText(tr("WBoard documents are present on your computer. It is possible to import them to WBoard by pressing the Proceed button to launch the importer application."));
    helpText->setAcceptDrops(false);
    helpText->setReadOnly(true);
    helpText->setStyleSheet("border : none;");
    mLayout->addWidget(helpText);

    mDisplayOnNextRestart = new QCheckBox(this);
    mDisplayOnNextRestart->setText(tr("Show this panel next time"));
    mDisplayOnNextRestart->setChecked(true);
    connect(mDisplayOnNextRestart,SIGNAL(clicked(bool)),this,SLOT(onNextRestartCheckBoxClicked(bool)));
    mLayout->addWidget(mDisplayOnNextRestart);
    mLayout->addSpacing(100);

    QTextEdit* warningText = new QTextEdit(this);
    warningText->setText(tr("You can always access the WBoard Document Importer through the Preferences panel in the About tab. Warning, if you have already imported your WBoard datas, you might loose your current WBoard documents."));
    warningText->setReadOnly(true);
    warningText->setStyleSheet("border : none;");
    mLayout->addWidget(warningText);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* mCancelButton = new QPushButton(this);
    mCancelButton->setText(tr("Cancel"));
    buttonLayout->addWidget(mCancelButton);
    buttonLayout->addStretch();
    connect(mCancelButton,SIGNAL(clicked()),this,SLOT(close()));

    mProceedButton = new QPushButton(this);
    mProceedButton->setText(tr("Proceed"));
    buttonLayout->addWidget(mProceedButton);

    mLayout->addLayout(buttonLayout);


    show();
}

void WBOpenSankoreImporterWidget::onNextRestartCheckBoxClicked(bool clicked)
{
    WBSettings::settings()->appLookForOpenSankoreInstall->setBool(clicked);
}

void WBOpenSankoreImporterWidget::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);
    adjustSizeAndPosition();
    move((parentWidget()->width() - width()) / 2, (parentWidget()->height() - height()) / 5);
}


int WBOpenSankoreImporterWidget::border()
{
    return 10;
}

