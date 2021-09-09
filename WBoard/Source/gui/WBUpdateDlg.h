#ifndef WBUPDATEDLG_H
#define WBUPDATEDLG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>

class WBUpdateDlg : public QDialog
{
    Q_OBJECT

public:
    WBUpdateDlg(QWidget *parent = 0, int nbFiles = 0, const QString& bkpPath = "");
    ~WBUpdateDlg();
    QString backupPath();

public slots:
    void onFilesUpdated(bool bResult);

signals:
    void updateFiles();

private slots:
    void onBrowse();
    void onUpdate();
    void transitioningFile(QString fileName);

private:
    QVBoxLayout* mMainLayout;

    QLabel* mNbFilesLabel;
    QLabel* mBkpLabel;
    QLineEdit* mBkpPath;
    QPushButton* mBrowseBttn;
    QDialogButtonBox* mpDlgBttn;
    QVBoxLayout* mLayout;
    QHBoxLayout* mHLayout;

    QStackedWidget* mStackedWidget;
    QWidget* mDialogWidget;
    QWidget* mProgressWidget;
    QHBoxLayout* mProgressLayout;
    QLabel* mProgressLabel;
};

#endif // WBUPDATEDLG_H
