#ifndef UBCOOKIEJAR_H
#define UBCOOKIEJAR_H

#include <QtNetwork/QNetworkCookieJar>

#include <QtGui>

class UBAutoSaver;

class UBCookieJar : public QNetworkCookieJar
{
    friend class UBCookieModel;
    Q_OBJECT
    Q_PROPERTY(AcceptPolicy acceptPolicy READ acceptPolicy WRITE setAcceptPolicy)
    Q_PROPERTY(KeepPolicy keepPolicy READ keepPolicy WRITE setKeepPolicy)
    Q_PROPERTY(QStringList blockedCookies READ blockedCookies WRITE setBlockedCookies)
    Q_PROPERTY(QStringList allowedCookies READ allowedCookies WRITE setAllowedCookies)
    Q_PROPERTY(QStringList allowForSessionCookies READ allowForSessionCookies WRITE setAllowForSessionCookies)
    Q_ENUMS(KeepPolicy)
    Q_ENUMS(AcceptPolicy)

    signals:
        void cookiesChanged();

    public:
        enum AcceptPolicy {
            AcceptAlways,
            AcceptNever,
            AcceptOnlyFromSitesNavigatedTo
        };

        enum KeepPolicy {
            KeepUntilExpire,
            KeepUntilExit,
            KeepUntilTimeLimit
        };

        UBCookieJar(QObject *parent = 0);
        ~UBCookieJar();

        QList<QNetworkCookie> cookiesForUrl(const QUrl &url) const;
        bool setCookiesFromUrl(const QList<QNetworkCookie> &cookieList, const QUrl &url);

        AcceptPolicy acceptPolicy() const;
        void setAcceptPolicy(AcceptPolicy policy);

        KeepPolicy keepPolicy() const;
        void setKeepPolicy(KeepPolicy policy);

        QStringList blockedCookies() const;
        QStringList allowedCookies() const;
        QStringList allowForSessionCookies() const;

        void setBlockedCookies(const QStringList &list);
        void setAllowedCookies(const QStringList &list);
        void setAllowForSessionCookies(const QStringList &list);

    public slots:
        void clear();
        void loadSettings();

    private slots:
        void save();

    private:
        void purgeOldCookies();
        void load();
        bool mLoaded;
        UBAutoSaver *mSaveTimer;

        AcceptPolicy mAcceptCookies;
        KeepPolicy mKeepCookies;

        QStringList mExceptionsBlock;
        QStringList mExceptionsAllow;
        QStringList mExceptionsAllowForSession;
};


#endif // UBCOOKIEJAR_H

