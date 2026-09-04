#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusReply>
#include <QScreen>
#include <QCursor>
#include <QDir>
#include <QIcon>
#include <iostream>

#include "core/SearchController.h"
#include "lua/LuaPluginManager.h"
#include "services/ThemeService.h"
#include "services/IconImageProvider.h"
#include "dbus/SpotlightAdaptor.h"

int main(int argc, char *argv[])
{
    // Ensure Wayland environment
    qputenv("QT_QPA_PLATFORM", "wayland");

    QGuiApplication app(argc, argv);
    app.setApplicationName("mvspotlight");
    app.setOrganizationName("MVSpotlight");
    app.setApplicationVersion("1.0.0");
    app.setWindowIcon(QIcon::fromTheme("mvspotlight", QIcon::fromTheme("system-search")));
    app.setQuitOnLastWindowClosed(false);

    QCommandLineParser parser;
    parser.setApplicationDescription("MVSpotlight: macOS Spotlight-Inspired Launcher for GNOME 50 (Wayland)");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption toggleOpt(QStringList() << "t" << "toggle", "Toggle launcher visibility");
    parser.addOption(toggleOpt);

    QCommandLineOption showOpt(QStringList() << "s" << "show", "Show launcher");
    parser.addOption(showOpt);

    QCommandLineOption hideOpt("hide", "Hide launcher");
    parser.addOption(hideOpt);

    QCommandLineOption searchOpt(QStringList() << "search", "Open launcher with query", "query");
    parser.addOption(searchOpt);

    QCommandLineOption reloadOpt(QStringList() << "r" << "reload-plugins", "Reload all Lua plugins");
    parser.addOption(reloadOpt);

    QCommandLineOption listOpt(QStringList() << "l" << "list-plugins", "List all installed plugins");
    parser.addOption(listOpt);

    QCommandLineOption daemonOpt(QStringList() << "d" << "daemon", "Start in background daemon mode (hidden)");
    parser.addOption(daemonOpt);

    parser.process(app);

    const QString serviceName = "org.mvspotlight.Launcher";
    const QString objectPath = "/org/mvspotlight/Launcher";
    const QString interfaceName = "org.mvspotlight.Launcher";

    const QString legacyServiceName = "org.example.Spotlight";
    const QString legacyObjectPath = "/org/example/Spotlight";
    const QString legacyInterfaceName = "org.example.Spotlight";

    QDBusConnection bus = QDBusConnection::sessionBus();

    // Check if another instance is already running (check new service first, then legacy)
    QString activeService;
    QString activePath;
    QString activeIface;
    if (bus.interface()) {
        if (bus.interface()->isServiceRegistered(serviceName)) {
            activeService = serviceName;
            activePath = objectPath;
            activeIface = interfaceName;
        } else if (bus.interface()->isServiceRegistered(legacyServiceName)) {
            activeService = legacyServiceName;
            activePath = legacyObjectPath;
            activeIface = legacyInterfaceName;
        }
    }

    if (!activeService.isEmpty()) {
        QDBusInterface iface(activeService, activePath, activeIface, bus);
        if (iface.isValid()) {
            if (parser.isSet(toggleOpt)) {
                iface.call("Toggle");
            } else if (parser.isSet(showOpt)) {
                iface.call("Show");
            } else if (parser.isSet(hideOpt)) {
                iface.call("Hide");
            } else if (parser.isSet(searchOpt)) {
                iface.call("Search", parser.value(searchOpt));
            } else if (parser.isSet(reloadOpt)) {
                iface.call("ReloadPlugins");
                std::cout << "Triggered plugin reload on active MVSpotlight instance." << std::endl;
            } else if (parser.isSet(listOpt)) {
                QDBusReply<QStringList> reply = iface.call("ListPlugins");
                if (reply.isValid()) {
                    for (const QString &p : reply.value()) {
                        std::cout << p.toStdString() << std::endl;
                    }
                }
            } else {
                // Default action when called without specific flags: toggle
                iface.call("Toggle");
            }
            return 0;
        }
    }

    // Register D-Bus services for single-instance
    if (!bus.registerService(serviceName)) {
        qWarning() << "Failed to register D-Bus service:" << serviceName << bus.lastError().message();
    }
    // Also register legacy service alias
    bus.registerService(legacyServiceName);

    // Initialize controller and services
    SearchController controller;
    controller.init();

    if (parser.isSet(listOpt)) {
        if (controller.pluginManager()) {
            for (const auto &p : controller.pluginManager()->plugins()) {
                std::cout << p.name().toStdString() << " [" << p.statusString().toStdString() << "] - "
                          << p.description().toStdString() << " (" << p.id().toStdString() << ")" << std::endl;
            }
        }
        return 0;
    }

    MVSpotlightAdaptor mvAdaptor(&controller);
    SpotlightAdaptor legacyAdaptor(&controller);
    if (!bus.registerObject(objectPath, &controller)) {
        qWarning() << "Failed to register D-Bus object at" << objectPath << bus.lastError().message();
    }
    bus.registerObject(legacyObjectPath, &controller);

    // Initialize QML Engine
    QQmlApplicationEngine engine;
    engine.addImageProvider("icon", new IconImageProvider());

    engine.rootContext()->setContextProperty("searchController", &controller);
    engine.rootContext()->setContextProperty("themeService", &ThemeService::instance());

    auto getRootWindow = [&engine]() -> QQuickWindow* {
        if (!engine.rootObjects().isEmpty()) {
            return qobject_cast<QQuickWindow*>(engine.rootObjects().first());
        }
        return nullptr;
    };

    auto updateScreenAndPosition = [&engine, getRootWindow]() {
        QQuickWindow *rootWindow = getRootWindow();
        if (!rootWindow) return;

        // Position on monitor with pointer
        QScreen *targetScreen = nullptr;
        QPoint cursorPos = QCursor::pos();
        for (QScreen *screen : QGuiApplication::screens()) {
            if (screen->geometry().contains(cursorPos)) {
                targetScreen = screen;
                break;
            }
        }
        if (!targetScreen) {
            targetScreen = QGuiApplication::primaryScreen();
        }

        if (targetScreen) {
            rootWindow->setScreen(targetScreen);
            QRect geo = targetScreen->geometry();
            int winW = rootWindow->width();
            int winH = rootWindow->height();
            int targetX = geo.x() + (geo.width() - winW) / 2;
            int targetY = geo.y() + static_cast<int>(geo.height() * 0.26);
            rootWindow->setGeometry(targetX, targetY, winW, winH);
        }
    };

    const QUrl url(QStringLiteral("qrc:/qml/Main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url, &controller, updateScreenAndPosition, getRootWindow](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) {
            QCoreApplication::exit(-1);
            return;
        }
        if (controller.isWindowVisible()) {
            updateScreenAndPosition();
            QQuickWindow *rootWindow = getRootWindow();
            if (rootWindow) {
                rootWindow->show();
                rootWindow->raise();
                rootWindow->requestActivate();
            }
        }
    }, Qt::QueuedConnection);

    engine.load(url);

    QObject::connect(&controller, &SearchController::windowVisibleChanged, [&controller, updateScreenAndPosition, getRootWindow]() {
        QQuickWindow *rootWindow = getRootWindow();
        if (controller.isWindowVisible()) {
            updateScreenAndPosition();
            if (rootWindow) {
                rootWindow->show();
                rootWindow->raise();
                rootWindow->requestActivate();
            }
        } else {
            if (rootWindow) {
                rootWindow->hide();
            }
        }
    });


    // Determine initial visibility
    if (parser.isSet(searchOpt)) {
        controller.setQuery(parser.value(searchOpt));
        controller.showWindow();
    } else if (parser.isSet(showOpt) || parser.isSet(toggleOpt)) {
        controller.showWindow();
    } else if (!parser.isSet(daemonOpt)) {
        // Normal interactive launch shows the window
        controller.showWindow();
    }

    return app.exec();
}
