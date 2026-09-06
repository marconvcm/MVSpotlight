#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QQuickItem>
#include <QDir>
#include <QFile>
#include <QThread>
#include <QImage>
#include <QDebug>
#include <iostream>

#include "core/SearchController.h"
#include "lua/LuaPluginManager.h"
#include "services/ThemeService.h"
#include "services/ConfigService.h"
#include "services/AiService.h"
#include "services/UsageHistory.h"
#include "services/ClipboardService.h"
#include "services/NotificationService.h"
#include "services/IconImageProvider.h"

static void processWait(int msec)
{
    for (int i = 0; i < msec / 10; ++i) {
        QThread::msleep(10);
        QCoreApplication::processEvents();
    }
}

static void saveWindow(QQuickWindow *win, const QString &filename)
{
    processWait(150);
    QImage img = win->grabWindow();
    if (!img.isNull()) {
        img.save(filename);
        std::cout << "  ✓ Captured: " << filename.toStdString()
                  << " (" << img.width() << "x" << img.height() << ")" << std::endl;
    } else {
        std::cerr << "  ✗ Failed to grab: " << filename.toStdString() << std::endl;
    }
}

int main(int argc, char *argv[])
{
    // Force offscreen headless virtual rendering with 2x High-DPI scaling
    qputenv("QT_QPA_PLATFORM", "offscreen");
    qputenv("QT_SCALE_FACTOR", "2");

    QGuiApplication app(argc, argv);
    app.setApplicationName("mvspotlight");

    QString outDir = "assets/screenshots";
    if (argc > 1) {
        outDir = argv[1];
    }
    QDir().mkpath(outDir);

    std::cout << "=================================================" << std::endl;
    std::cout << " MVSpotlight Virtual Desktop Showcase Capturer   " << std::endl;
    std::cout << " Output: " << outDir.toStdString() << std::endl;
    std::cout << "=================================================" << std::endl;

    SearchController controller;
    if (!controller.init()) {
        std::cerr << "Failed to init SearchController" << std::endl;
        return 1;
    }

    // Configure Dark theme and standard styling defaults
    ConfigService &cfg = ConfigService::instance();
    cfg.setThemeMode("dark");
    cfg.setAccentColor("#3584e4");
    cfg.setSearchDebounceMs(0); // instant search for screenshot capture

    QQmlApplicationEngine engine;
    engine.addImageProvider(QLatin1String("icon"), new IconImageProvider());
    engine.rootContext()->setContextProperty("searchController", &controller);
    engine.rootContext()->setContextProperty("themeService", &ThemeService::instance());
    engine.rootContext()->setContextProperty("configService", &ConfigService::instance());
    engine.rootContext()->setContextProperty("aiService", &AiService::instance());
    engine.rootContext()->setContextProperty("usageHistory", &UsageHistory::instance());
    engine.rootContext()->setContextProperty("clipboardService", &ClipboardService::instance());
    engine.rootContext()->setContextProperty("notificationService", &NotificationService::instance());
    engine.rootContext()->setContextProperty("pluginManager", controller.pluginManager());

    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
    if (engine.rootObjects().isEmpty()) {
        std::cerr << "Failed to load Main.qml" << std::endl;
        return 1;
    }

    QQuickWindow *mainWindow = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
    if (!mainWindow) {
        std::cerr << "Main.qml root is not a QQuickWindow" << std::endl;
        return 1;
    }

    // -------------------------------------------------------------
    // 1. Hero Search Bar (Clean floating launcher)
    // -------------------------------------------------------------
    std::cout << "Capturing Hero Spotlight Bar..." << std::endl;
    controller.setQuery("");
    controller.setWindowVisible(true);
    mainWindow->show();
    saveWindow(mainWindow, outDir + "/hero_search.png");

    // -------------------------------------------------------------
    // 2. Application Search Results
    // -------------------------------------------------------------
    std::cout << "Capturing Application Search..." << std::endl;
    controller.setQuery("terminal");
    controller.flushSearch();
    processWait(200);
    saveWindow(mainWindow, outDir + "/search_apps.png");

    // -------------------------------------------------------------
    // 2b. GNOME Settings Search Results
    // -------------------------------------------------------------
    std::cout << "Capturing Settings Search..." << std::endl;
    controller.setQuery("settings");
    controller.flushSearch();
    processWait(200);
    saveWindow(mainWindow, outDir + "/search_settings.png");

    // -------------------------------------------------------------
    // 3. Calculator / Expression Evaluator
    // -------------------------------------------------------------
    std::cout << "Capturing Calculator..." << std::endl;
    controller.setQuery("sqrt(144) + 25 * 4");
    controller.flushSearch();
    saveWindow(mainWindow, outDir + "/search_calculator.png");

    // -------------------------------------------------------------
    // 4. Currency Exchange Plugin
    // -------------------------------------------------------------
    std::cout << "Capturing Currency Exchange..." << std::endl;
    controller.setQuery("100 eur to brl");
    controller.flushSearch();
    saveWindow(mainWindow, outDir + "/search_currency.png");

    // -------------------------------------------------------------
    // 5. AI Assistant Query Card & Response
    // -------------------------------------------------------------
    std::cout << "Capturing AI Assistant Mode..." << std::endl;
    QString aiPrompt = "difference between mutex and semaphore";
    QString aiAnswer =
        "### Key Differences: Mutex vs. Semaphore\n\n"
        "| Feature | **Mutex** (Mutual Exclusion) | **Semaphore** (Signaling) |\n"
        "| :--- | :--- | :--- |\n"
        "| **Mechanism** | Locking mechanism | Signaling mechanism |\n"
        "| **Ownership** | Locked and unlocked by **same thread** | Can be signaled by **any thread** |\n"
        "| **Capacity** | Single resource (value is 0 or 1) | Counting integer (multiple resources) |\n"
        "| **Use Case** | Serializing access to shared data | Producer-consumer queues, rate limiting |\n\n"
        "```cpp\n"
        "// Mutex example:\n"
        "std::mutex mtx;\n"
        "mtx.lock();\n"
        "critical_section();\n"
        "mtx.unlock();\n"
        "```\n\n"
        "- **Mutex**: Think of it as a key to a single bathroom stall.\n"
        "- **Counting Semaphore**: Think of it as an attendant managing passes for multiple stalls.";

    controller.setQuery("> " + aiPrompt);
    controller.flushSearch();
    AiService::instance().setMockResponse(aiPrompt, aiAnswer);
    processWait(200);
    saveWindow(mainWindow, outDir + "/ai_chat.png");

    // Hide main window before capturing preferences
    controller.setWindowVisible(false);
    mainWindow->hide();
    processWait(50);

    // -------------------------------------------------------------
    // 6. Preferences Window (Appearance, AI, Plugins)
    // -------------------------------------------------------------
    QQmlComponent prefComponent(&engine, QUrl(QStringLiteral("qrc:/qml/PreferencesWindow.qml")));
    QObject *prefObj = prefComponent.create();
    QQuickWindow *prefWindow = qobject_cast<QQuickWindow*>(prefObj);

    if (prefWindow) {
        prefWindow->show();

        // 6a. Appearance Tab (with live preview)
        std::cout << "Capturing Preferences Appearance..." << std::endl;
        prefWindow->setProperty("currentTab", 0);
        saveWindow(prefWindow, outDir + "/preferences_appearance.png");

        // 6b. AI Assistant Tab
        std::cout << "Capturing Preferences AI..." << std::endl;
        prefWindow->setProperty("currentTab", 1);
        saveWindow(prefWindow, outDir + "/preferences_ai.png");

        // 6c. Plugins Tab
        std::cout << "Capturing Preferences Plugins..." << std::endl;
        prefWindow->setProperty("currentTab", 2);
        saveWindow(prefWindow, outDir + "/preferences_plugins.png");

        prefWindow->hide();
    } else {
        std::cerr << "Could not instantiate PreferencesWindow" << std::endl;
    }

    std::cout << "=================================================" << std::endl;
    std::cout << " All screenshots generated successfully!        " << std::endl;
    std::cout << "=================================================" << std::endl;

    return 0;
}
