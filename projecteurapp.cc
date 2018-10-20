#include "projecteurapp.h"

#include "qglobalshortcutx11.h"

#include <QMenu>
#include <QMessageBox>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QScopedPointer>
#include <QScreen>
#include <QSystemTrayIcon>
#include <QTimer>

#include <QDebug>

ProjecteurApplication::ProjecteurApplication(int &argc, char **argv)
  : QApplication(argc, argv)
  , m_trayIcon(new QSystemTrayIcon(this))
  , m_trayMenu(new QMenu())
{
  auto engine = new QQmlApplicationEngine(QUrl(QStringLiteral("qrc:/main.qml")), this);
  auto window = qobject_cast<QQuickWindow*>(engine->rootObjects().first());

  if (!window)
  {
    QMessageBox::critical(nullptr, tr("No root window found"),
                                   tr("First item in QML hierarchy must be a Window."));
    QTimer::singleShot(0, [this](){ this->exit(1); });
    return;
  }

  if (screens().size() < 1)
  {
    QMessageBox::critical(nullptr, tr("No Screens"), tr("screens().size() returned a size < 1."));
    QTimer::singleShot(0, [this](){ this->exit(2); });
    return;
  }

  auto setFlags = [window]() {
    window->setFlags(window->flags() | Qt::WindowTransparentForInput | Qt::Tool);
    window->hide();
  };

  // /sys/class/input/event18/device/id
  // parse /proc/bus/input/devices -
  //    https://unix.stackexchange.com/questions/74903/explain-ev-in-proc-bus-input-devices-data/74907#74907

  // TODO: Set screen set in options or command line if available use first screen as fallback.
  // TODO: Notify if set screen is not availabe and notify of fallback.
  const auto availGeometry = screens().first()->availableGeometry();

  // It seems we need to set the transparent and tool flags AFTER the window size has changed,
  // otherwise it will not work.
  window->connect(window, &QWindow::heightChanged, [availGeometry, window, setFlags](int h) {
    if(availGeometry.height() == h && window->width() == availGeometry.width() ) { setFlags(); }
  });
  window->connect(window, &QWindow::widthChanged, [availGeometry, window, setFlags](int w) {
    if(availGeometry.width() == w && window->height() == availGeometry.height() ) { setFlags(); }
  });

  window->setGeometry(availGeometry);

  const auto shortcut = new QGlobalShortcutX11(QKeySequence("Ctrl+F3"), this);
  connect(shortcut, &QGlobalShortcutX11::activated, [window](){
    if(window->flags() & Qt::WindowTransparentForInput)
    {
      qDebug() << "activated";
      window->setFlag(Qt::WindowTransparentForInput, false);
      window->show();
    }
    else {
      qDebug() << "de-activated";
      window->setFlag(Qt::WindowTransparentForInput, true);
      window->hide();
    }
  });

  m_trayIcon->setIcon(QIcon(":/icons/projecteur-tray.svg"));
  m_trayIcon->show();

  m_trayMenu->addAction("Test1", [](){ qDebug() << "Test1";});
  m_trayIcon->setContextMenu( m_trayMenu.data() );

  //connect(m_trayIcon, &QSystemTrayIcon::activated...)
  //m_trayIcon->showMessage("Title", "Message....", QSystemTrayIcon::Information, 10000);

  // TODO: Change window size if available geometry changes...
  // QObject::connect(screens().first(), &QScreen::availableGeometryChanged, [](const QRect& /*g*/){});

  const auto shortcut2 = new QGlobalShortcutX11(QKeySequence("Ctrl+Alt+7"), this);
  connect(shortcut2, &QGlobalShortcutX11::activated, [this, window](){
    qDebug() << "activated ctrl+alt+7";
    if(window) window->close();
    this->quit();
  });

  //window->installEventFilter(new MyFilter(this));
}

ProjecteurApplication::~ProjecteurApplication()
{
}
