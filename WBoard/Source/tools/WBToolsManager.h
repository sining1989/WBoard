#ifndef WBTOOLSMANAGER_H_
#define WBTOOLSMANAGER_H_

#include <QtWidgets>

#include "core/WBApplication.h"

class WBToolsManager : public QObject
{
    Q_OBJECT

public:
    class WBToolDescriptor
    {
        public:
            QString id;
            QPixmap icon;
            QString label;
            QString version;

    };

    static WBToolsManager* manager();
    static void destroy();

    QList<WBToolDescriptor> allTools()
    {
            return mDescriptors;
    }

    QStringList allToolIDs()
    {
        QStringList ids;

        foreach(WBToolDescriptor tool, allTools())
        {
            ids << tool.id;
        }

        return ids;
    }

    WBToolDescriptor toolByID(const QString& id)
    {
        foreach(WBToolDescriptor tool, allTools())
        {
            if (tool.id == id)
                return tool;
        }

        return WBToolDescriptor();
    }

    WBToolDescriptor ruler;
    WBToolDescriptor protractor;
    WBToolDescriptor compass;
    WBToolDescriptor mask;
    WBToolDescriptor triangle;
    WBToolDescriptor magnifier;
    WBToolDescriptor cache;

    QString iconFromToolId(QString id) { return mToolsIcon.value(id);}

private:
    WBToolsManager(QObject *parent = 0);
    virtual ~WBToolsManager();

    static WBToolsManager* sManager;

    QList<WBToolDescriptor> mDescriptors;

    QMap<QString ,QString> mToolsIcon;

};

#endif /* WBTOOLSMANAGER_H_ */
