#ifndef WBOPENSANKOREIMPORTER_H
#define WBOPENSANKOREIMPORTER_H

class WBOpenSankoreImporterWidget;

#include <QObject>

class WBOpenSankoreImporter : public QObject
{
    Q_OBJECT
public:
    explicit WBOpenSankoreImporter(QWidget *mainWidget, QObject *parent = 0);

public slots:
    void onProceedClicked();

private:
    WBOpenSankoreImporterWidget* mImporterWidget;

};

#endif // WBOPENSANKOREIMPORTER_H
