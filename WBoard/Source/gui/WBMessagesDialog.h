#ifndef WB_MESSAGES_DIALOG_H_
#define WB_MESSAGES_DIALOG_H_

#include <QtWidgets>
#include <QWidget>

class WBMessagesDialog : public QWidget
{
    Q_OBJECT

public:
    WBMessagesDialog(QString windowTitle, QWidget *parent = NULL);
    void setMessages(const QList<QString> messages);

private slots:
    void dispose();

private:
    QList<QString> mMessages;
    int mMessagesFontSize;
};

#endif /* WB_MESSAGES_DIALOG_H_ */
