#ifndef WBDOCUMENTPROXY_H_
#define WBDOCUMENTPROXY_H_

#include <QtWidgets>

#include "frameworks/WBStringUtils.h"

#include "core/WBSettings.h"

class WBGraphicsScene;

class WBDocumentProxy : public QObject
{
    Q_OBJECT

	friend class WBPersistenceManager;

	public:
		WBDocumentProxy();
		WBDocumentProxy(const WBDocumentProxy &rValue);
		WBDocumentProxy(const QString& pPersistencePath);
		WBDocumentProxy(const QString& pPersistencePath, QMap<QString, QVariant> metadatas);

		virtual ~WBDocumentProxy();

		WBDocumentProxy * deepCopy() const;
		bool theSameDocument(WBDocumentProxy *proxy);

		QString persistencePath() const;

		void setPersistencePath(const QString& pPersistencePath);

		void setMetaData(const QString& pKey , const QVariant& pValue);
		QVariant metaData(const QString& pKey) const;
		QMap<QString, QVariant> metaDatas() const;

		QString name() const;
		QString groupName() const;
		QDateTime documentDate();

		QDateTime lastUpdate();

		QSize defaultDocumentSize() const;
		void setDefaultDocumentSize(QSize pSize);
		void setDefaultDocumentSize(int pWidth, int pHeight);

		QUuid uuid() const;
		void setUuid(const QUuid& uuid);

		bool isModified() const;

		int pageCount();

		int pageDpi();
		void setPageDpi(int dpi);

	protected:
		void setPageCount(int pPageCount);
		int incPageCount();
		int decPageCount();

	private:
		void init();

		QString mPersistencePath;

		QMap<QString, QVariant> mMetaDatas;

		bool mIsModified;

		int mPageCount;

		int mPageDpi;

};

inline bool operator==(const WBDocumentProxy &proxy1, const WBDocumentProxy &proxy2)
{
    return proxy1.persistencePath() == proxy2.persistencePath();
}

inline uint qHash(const WBDocumentProxy &key)
{
    return qHash(key.persistencePath());
}

#endif /* WBDOCUMENTPROXY_H_ */
