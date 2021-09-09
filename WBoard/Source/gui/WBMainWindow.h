#ifndef UBMAINWINDOW_H_
#define UBMAINWINDOW_H_

#include <QMainWindow>
#include <QWidget>
#include <QtWebEngineWidgets/qwebengineview.h>
#include <QMessageBox>
#include "WBDownloadWidget.h"

class QStackedLayout;

#include "ui_mainWindow.h"

class WBMainWindow : public QMainWindow, public Ui::MainWindow
{
    Q_OBJECT
public:

	WBMainWindow(QWidget *parent = 0, Qt::WindowFlags flags = 0);
	virtual ~WBMainWindow();

	void addBoardWidget(QWidget *pWidget);
	void switchToBoardWidget();

	void addWebWidget(QWidget *pWidget);
	void switchToWebWidget();

	void addDocumentsWidget(QWidget *pWidget);
	void switchToDocumentsWidget();

	bool yesNoQuestion(QString windowTitle, QString text);
	void warning(QString windowTitle, QString text);
	void information(QString windowTitle, QString text);

	void showDownloadWidget();
	void hideDownloadWidget();

signals:
	void closeEvent_Signal(QCloseEvent *event);

public slots:
	void onExportDone();

protected:
	void oneButtonMessageBox(QString windowTitle, QString text, QMessageBox::Icon type);

	virtual void keyPressEvent(QKeyEvent *event);
	virtual void closeEvent (QCloseEvent *event);

	virtual QMenu* createPopupMenu()
	{
		return 0;
	}

	QStackedLayout* mStackedLayout;

	QWidget *mBoardWidget;
	QWidget *mWebWidget;
	QWidget *mDocumentsWidget;

private:
#if defined(Q_OS_OSX)
	bool event(QEvent *event);
#endif
	WBDownloadWidget* mpDownloadWidget;
};

#endif /* WBMAINWINDOW_H_ */
