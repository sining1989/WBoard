#ifndef WBSETTING_H_
#define WBSETTING_H_

#include <QtWidgets>

class WBSettings;

class WBSetting : public QObject
{
    Q_OBJECT

    public:
        WBSetting(WBSettings* parent = 0);
        WBSetting(WBSettings* owner, const QString& pDomain, const QString& pKey,
                        const QVariant& pDefaultValue);

        virtual ~WBSetting();

        virtual void set(const QVariant& pValue);
        virtual QVariant get();
        virtual QVariant reset();

        virtual QString domain() const
        {
            return mDomain;
        }

        virtual QString key() const
        {
            return mKey;
        }

        virtual QString path() const
        {
            return mPath;
        }

        virtual QVariant defaultValue() const
        {
            return mDefaultValue;
        }

    public slots:
        void setBool(bool pValue);
        void setString(const QString& pValue);
        void setInt(int pValue);

    signals:
        void changed(QVariant newValue);

    protected:
        WBSettings* mOwner;
        QString mDomain;
        QString mKey;
        QString mPath;
        QVariant mDefaultValue;
};

class WBColorListSetting : public WBSetting
{
    Q_OBJECT

    public:
        WBColorListSetting(WBSettings* parent = 0);
        WBColorListSetting(WBSettings* owner, const QString& pDomain,
                const QString& pKey, const QVariant& pDefaultValue, qreal pAlpha = 1.0);

        virtual ~WBColorListSetting();

        virtual QVariant reset();

        QList<QColor> colors() const;

        void setColor(int pIndex, const QColor& color);

        void setAlpha(qreal pAlpha);

    protected:
        QList<QColor> mColors;
        qreal mAlpha;
};
#endif /* WBSETTING_H_ */
