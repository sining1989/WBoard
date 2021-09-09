#include <QLayout>
#include <QTextEdit>
#include <QPushButton>

#include "WBMessagesDialog.h"

WBMessagesDialog::WBMessagesDialog(QString windowTitle, QWidget *parent)
: QWidget(parent)
{
    resize(400, 0);

    setWindowTitle(windowTitle);
    setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);

}

void WBMessagesDialog::setMessages(const QList<QString> messages)
{
    mMessages = messages;

    if (mMessages.count())
    {        
        QVBoxLayout *messagesLayout = new QVBoxLayout(this);
        foreach (QString message, mMessages)
        {
            QTextEdit *messageBox = new QTextEdit(this);
            messageBox->setMinimumHeight(55);
            messageBox->setReadOnly(true);
            messageBox->setFocusPolicy(Qt::NoFocus);
            messageBox->setText(message);
            messagesLayout->addWidget(messageBox);
        }
        QPushButton *closeButton = new QPushButton(tr("Close"), this);
        connect(closeButton, SIGNAL(clicked(bool)), this, SLOT(dispose()));

        messagesLayout->addWidget(closeButton);

        setLayout(messagesLayout);
    }
}

void WBMessagesDialog::dispose()
{
    delete this;
}
