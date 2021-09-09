#include "qtsingleapplication.h"
#include "qtlocalpeer.h"
#include <QWidget>

void QtSingleApplication::sysInit(const QString &appId)
{
    actWin = 0;
    peer = new QtLocalPeer(this, appId);
    connect(peer, SIGNAL(messageReceived(const QString&)), SIGNAL(messageReceived(const QString&)));
}


/*!
    Creates a QtSingleApplication object. The application identifier
    will be QCoreApplication::applicationFilePath(). \a argc, \a
    argv, and \a GUIenabled are passed on to the QAppliation constructor.

    If you are creating a console application (i.e. setting \a
    GUIenabled to false), you may consider using
    QtSingleCoreApplication instead.
*/

QtSingleApplication::QtSingleApplication(int &argc, char **argv, bool GUIenabled)
    : QApplication(argc, argv, GUIenabled)
{
    sysInit();
}


/*!
    Creates a QtSingleApplication object with the application
    identifier \a appId. \a argc and \a argv are passed on to the
    QAppliation constructor.
*/

QtSingleApplication::QtSingleApplication(const QString &appId, int &argc, char **argv)
    : QApplication(argc, argv)
{
    sysInit(appId);
}

#if QT_VERSION < 0x050000

/*!
    Creates a QtSingleApplication object. The application identifier
    will be QCoreApplication::applicationFilePath(). \a argc, \a
    argv, and \a type are passed on to the QAppliation constructor.
*/
QtSingleApplication::QtSingleApplication(int &argc, char **argv, Type type)
    : QApplication(argc, argv, type)
{
    sysInit();
}


#  if defined(Q_WS_X11)
/*!
  Special constructor for X11, ref. the documentation of
  QApplication's corresponding constructor. The application identifier
  will be QCoreApplication::applicationFilePath(). \a dpy, \a visual,
  and \a cmap are passed on to the QApplication constructor.
*/
QtSingleApplication::QtSingleApplication(Display* dpy, Qt::HANDLE visual, Qt::HANDLE cmap)
    : QApplication(dpy, visual, cmap)
{
    sysInit();
}

/*!
  Special constructor for X11, ref. the documentation of
  QApplication's corresponding constructor. The application identifier
  will be QCoreApplication::applicationFilePath(). \a dpy, \a argc, \a
  argv, \a visual, and \a cmap are passed on to the QApplication
  constructor.
*/
QtSingleApplication::QtSingleApplication(Display *dpy, int &argc, char **argv, Qt::HANDLE visual, Qt::HANDLE cmap)
    : QApplication(dpy, argc, argv, visual, cmap)
{
    sysInit();
}

/*!
  Special constructor for X11, ref. the documentation of
  QApplication's corresponding constructor. The application identifier
  will be \a appId. \a dpy, \a argc, \a
  argv, \a visual, and \a cmap are passed on to the QApplication
  constructor.
*/
QtSingleApplication::QtSingleApplication(Display* dpy, const QString &appId, int argc, char **argv, Qt::HANDLE visual, Qt::HANDLE cmap)
    : QApplication(dpy, argc, argv, visual, cmap)
{
    sysInit(appId);
}
#  endif // Q_WS_X11
#endif // QT_VERSION < 0x050000


/*!
    Returns true if another instance of this application is running;
    otherwise false.

    This function does not find instances of this application that are
    being run by a different user (on Windows: that are running in
    another session).

    \sa sendMessage()
*/

bool QtSingleApplication::isRunning()
{
    return peer->isClient();
}


/*!
    Tries to send the text \a message to the currently running
    instance. The QtSingleApplication object in the running instance
    will emit the messageReceived() signal when it receives the
    message.

    This function returns true if the message has been sent to, and
    processed by, the current instance. If there is no instance
    currently running, or if the running instance fails to process the
    message within \a timeout milliseconds, this function return false.

    \sa isRunning(), messageReceived()
*/
bool QtSingleApplication::sendMessage(const QString &message, int timeout)
{
    return peer->sendMessage(message, timeout);
}


/*!
    Returns the application identifier. Two processes with the same
    identifier will be regarded as instances of the same application.
*/
QString QtSingleApplication::id() const
{
    return peer->applicationId();
}


/*!
  Sets the activation window of this application to \a aw. The
  activation window is the widget that will be activated by
  activateWindow(). This is typically the application's main window.

  If \a activateOnMessage is true (the default), the window will be
  activated automatically every time a message is received, just prior
  to the messageReceived() signal being emitted.

  \sa activateWindow(), messageReceived()
*/

void QtSingleApplication::setActivationWindow(QWidget* aw, bool activateOnMessage)
{
    actWin = aw;
    if (activateOnMessage)
        connect(peer, SIGNAL(messageReceived(const QString&)), this, SLOT(activateWindow()));
    else
        disconnect(peer, SIGNAL(messageReceived(const QString&)), this, SLOT(activateWindow()));
}


/*!
    Returns the applications activation window if one has been set by
    calling setActivationWindow(), otherwise returns 0.

    \sa setActivationWindow()
*/
QWidget* QtSingleApplication::activationWindow() const
{
    return actWin;
}


/*!
  De-minimizes, raises, and activates this application's activation window.
  This function does nothing if no activation window has been set.

  This is a convenience function to show the user that this
  application instance has been activated when he has tried to start
  another instance.

  This function should typically be called in response to the
  messageReceived() signal. By default, that will happen
  automatically, if an activation window has been set.

  \sa setActivationWindow(), messageReceived(), initialize()
*/
void QtSingleApplication::activateWindow()
{
    if (actWin) {
        actWin->setWindowState(actWin->windowState() & ~Qt::WindowMinimized);
        actWin->raise();
        actWin->activateWindow();
    }
}


/*!
    \fn void QtSingleApplication::messageReceived(const QString& message)

    This signal is emitted when the current instance receives a \a
    message from another instance of this application.

    \sa sendMessage(), setActivationWindow(), activateWindow()
*/


/*!
    \fn void QtSingleApplication::initialize(bool dummy = true)

    \obsolete
*/
