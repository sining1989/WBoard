#include "WBW3CWidgetAPI.h"

#include <QtWidgets>
#include <QtWebEngine>

#include "core/WBApplication.h"
#include "core/WBApplicationController.h"

#include "web/WBWebController.h"

#include "domain/WBGraphicsWidgetItem.h"
#include "domain/WBGraphicsProxyWidget.h"

#include "WBWidgetMessageAPI.h"

#include "core/memcheck.h"

WBW3CWidgetAPI::WBW3CWidgetAPI(WBGraphicsW3CWidgetItem *graphicsWidget, QObject *parent)
    : QObject(parent)
    , mGraphicsW3CWidget(graphicsWidget)
{
    mPreferencesAPI = new WBW3CWidgetPreferenceAPI(graphicsWidget, parent);

}

WBW3CWidgetAPI::~WBW3CWidgetAPI()
{
    // NOOP
}


QString WBW3CWidgetAPI::uuid()
{
    if (mGraphicsW3CWidget)
        return mGraphicsW3CWidget->uuid().toString();
    else
        return "";
}


int WBW3CWidgetAPI::width()
{
    return mGraphicsW3CWidget->nominalSize().width();
}


int WBW3CWidgetAPI::height()
{
    return mGraphicsW3CWidget->nominalSize().height();
}


QString WBW3CWidgetAPI::id()
{
    return mGraphicsW3CWidget->metadatas().id;
}


QString WBW3CWidgetAPI::name()
{
    return mGraphicsW3CWidget->metadatas().name;
}


QString WBW3CWidgetAPI::description()
{
    return mGraphicsW3CWidget->metadatas().description;
}


QString WBW3CWidgetAPI::author()
{
    return mGraphicsW3CWidget->metadatas().author;
}


QString WBW3CWidgetAPI::authorEmail()
{
    return mGraphicsW3CWidget->metadatas().authorEmail;
}


QString WBW3CWidgetAPI::authorHref()
{
    return mGraphicsW3CWidget->metadatas().authorHref;
}


QString WBW3CWidgetAPI::version()
{
    return mGraphicsW3CWidget->metadatas().version;
}

QObject* WBW3CWidgetAPI::preferences()
{
    return mPreferencesAPI;
}


void WBW3CWidgetAPI::openURL(const QString& url)
{
    WBApplication::webController->loadUrl(QUrl(url));
}


WBW3CWidgetPreferenceAPI::WBW3CWidgetPreferenceAPI(WBGraphicsW3CWidgetItem *graphicsWidget, QObject *parent)
    : WBW3CWebStorage(parent)
    , mGraphicsW3CWidget(graphicsWidget)
{
    // NOOP
}

WBW3CWidgetPreferenceAPI::~WBW3CWidgetPreferenceAPI()
{
    // NOOP
}


QString WBW3CWidgetPreferenceAPI::key(int index)
{
  QMap<QString, WBGraphicsW3CWidgetItem::PreferenceValue> w3CPrefs = mGraphicsW3CWidget->preferences();

  if (index < w3CPrefs.size())
    return w3CPrefs.keys().at(index);
  else
    return "";
}

QString WBW3CWidgetPreferenceAPI::getItem(const QString& key)
{
  if (mGraphicsW3CWidget) {
    QMap<QString, QString> docPref = mGraphicsW3CWidget->WBGraphicsWidgetItem::preferences();
    if (docPref.contains(key))
      return docPref.value(key);
  

    QMap<QString, WBGraphicsW3CWidgetItem::PreferenceValue> w3cPrefs = mGraphicsW3CWidget->preferences();

    if (w3cPrefs.contains(key)) {
      WBGraphicsW3CWidgetItem::PreferenceValue pref = w3cPrefs.value(key);
      return pref.value;
    }
  }
  return QString();
}

int WBW3CWidgetPreferenceAPI::length()
{
   QMap<QString, WBGraphicsW3CWidgetItem::PreferenceValue> w3cPrefs = mGraphicsW3CWidget->preferences();

   return w3cPrefs.size();
}


void WBW3CWidgetPreferenceAPI::setItem(const QString& key, const QString& value)
{
  if (mGraphicsW3CWidget) {
    QMap<QString, WBGraphicsW3CWidgetItem::PreferenceValue> w3cPrefs = mGraphicsW3CWidget->preferences();

    if (w3cPrefs.contains(key) && !w3cPrefs.value(key).readonly)
      mGraphicsW3CWidget->setPreference(key, value);
  }
}




void WBW3CWidgetPreferenceAPI::removeItem(const QString& key)
{
  if (mGraphicsW3CWidget)
    mGraphicsW3CWidget->removePreference(key);
}


void WBW3CWidgetPreferenceAPI::clear()
{
  if (mGraphicsW3CWidget)
    mGraphicsW3CWidget->removeAllPreferences();
}




