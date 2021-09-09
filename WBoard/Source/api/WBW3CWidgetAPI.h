#ifndef WBW3CWIDGETAPI_H_
#define WBW3CWIDGETAPI_H_

#include <QtCore>

class WBGraphicsW3CWidgetItem;
class WBW3CWidgetPreferenceAPI;
class WBW3CWidget;

class WBW3CWidgetAPI : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString id READ id SCRIPTABLE true);
    Q_PROPERTY(QString name READ name SCRIPTABLE true);
    Q_PROPERTY(QString description READ description SCRIPTABLE true);
    Q_PROPERTY(QString author READ author SCRIPTABLE true);

    Q_PROPERTY(QString authorEmail READ authorEmail SCRIPTABLE true);
    Q_PROPERTY(QString authorHref READ authorHref SCRIPTABLE true);
    Q_PROPERTY(QString version READ version SCRIPTABLE true);

    Q_PROPERTY(int width READ width SCRIPTABLE true);
    Q_PROPERTY(int height READ height SCRIPTABLE true);

    Q_PROPERTY(QObject* preferences READ preferences SCRIPTABLE true);

    // Mnemis extensions
    Q_PROPERTY(QString uuid READ uuid SCRIPTABLE true);

    public:
        WBW3CWidgetAPI(WBGraphicsW3CWidgetItem *graphicsWidget, QObject *parent = 0);

        virtual ~WBW3CWidgetAPI();

        QString uuid();

        QString id();
        QString name();
        QString description();
        QString author();
        QString authorEmail();
        QString authorHref();
        QString version();

        QObject* preferences();

        int width();
        int height();

        void openURL(const QString& url);

    private:
        WBGraphicsW3CWidgetItem* mGraphicsW3CWidget;
        WBW3CWidgetPreferenceAPI* mPreferencesAPI;
};

class WBW3CWebStorage : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int length READ length SCRIPTABLE true);

    public:
        WBW3CWebStorage(QObject *parent = 0)
        : QObject(parent){/* NOOP */}
        virtual ~WBW3CWebStorage(){/* NOOP */}

    public slots:

        virtual QString key(int index) = 0;
        virtual QString getItem(const QString& key) = 0;
        virtual void setItem(const QString& key, const QString& value) = 0;
        virtual void removeItem(const QString& key) = 0;
        virtual void clear() = 0;

    protected:
        virtual int length() = 0;

};

class WBW3CWidgetPreferenceAPI : public WBW3CWebStorage
{
    Q_OBJECT

    public:
        WBW3CWidgetPreferenceAPI(WBGraphicsW3CWidgetItem *graphicsWidget, QObject *parent = 0);

        virtual ~WBW3CWidgetPreferenceAPI();

    public slots:
        virtual QString key(int index);
        virtual QString getItem(const QString& key);
        virtual void setItem(const QString& key, const QString& value);
        virtual void removeItem(const QString& key);
        virtual void clear();

    protected:
        virtual int length();

    private:
        WBGraphicsW3CWidgetItem* mGraphicsW3CWidget;

};
#endif /* WBW3CWIDGETAPI_H_ */
