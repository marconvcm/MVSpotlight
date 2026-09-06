#include "DeveloperCommandProvider.h"
#include "../lua/LuaPluginManager.h"
#include "../services/NotificationService.h"
#include "../services/ThemeService.h"
#include "../services/ClipboardService.h"
#include <QGuiApplication>
#include <QScreen>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>

DeveloperCommandProvider::DeveloperCommandProvider(QObject *parent)
    : SearchProvider(parent)
{
}

QList<SearchResult> DeveloperCommandProvider::search(const QString &query)
{
    QList<SearchResult> results;
    QString q = query.trimmed().toLower();

    if (q == ":settings" || q == ":config" || q == "settings" || q == "preferences" ||
        q == "spotlight settings" || q == "spotlight preferences" || q == "configure spotlight" ||
        q == ":preferences" || q == "appearance" || q == "customize") {
        SearchResult sr;
        sr.setId("dev:preferences");
        sr.setTitle("MVSpotlight Preferences");
        sr.setSubtitle("Customize themes, accents, card dimensions, and plugins");
        sr.setIcon("preferences-system");
        sr.setScore(99.0);
        sr.setType("Preferences");
        sr.setProvider("Developer Tools");
        sr.setAction("open_preferences");
        results.append(sr);
    }

    if (q == ":plugins" || q == "plugins" || q == ":plugin") {
        SearchResult sr;
        sr.setId("dev:plugins");
        sr.setTitle("Manage Lua Plugins");
        sr.setSubtitle("Inspect loaded Lua plugins, statuses, and permissions");
        sr.setIcon("system-software-install");
        sr.setScore(100.0);
        sr.setType("Developer");
        sr.setProvider("Developer Tools");
        sr.setAction("open_preferences");
        results.append(sr);
    }

    if (q == ":reload" || q == "reload" || q == "reload plugins" || q == ":reload-plugins") {
        SearchResult sr;
        sr.setId("dev:reload");
        sr.setTitle("Reload All Plugins");
        sr.setSubtitle("Hot-reload Lua plugins without restarting MVSpotlight");
        sr.setIcon("view-refresh");
        sr.setScore(100.0);
        sr.setType("Developer");
        sr.setProvider("Developer Tools");
        sr.setAction("reload_plugins");
        results.append(sr);
    }

    if (q == ":logs" || q == "logs" || q == ":log") {
        SearchResult sr;
        sr.setId("dev:logs");
        sr.setTitle("Open Launcher Logs");
        sr.setSubtitle("View ~/.local/state/mvspotlight/launcher.log");
        sr.setIcon("text-x-generic");
        sr.setScore(100.0);
        sr.setType("Developer");
        sr.setProvider("Developer Tools");
        sr.setAction("open_logs");
        sr.setSecondaryActionLabel("Copy Log Path");
        sr.setSecondaryAction("copy_log_path");
        results.append(sr);
    }

    if (q == ":debug" || q == "debug") {
        SearchResult sr;
        sr.setId("dev:debug");
        sr.setTitle("Debug System Information");
        sr.setSubtitle("Wayland session, screen geometry, and Qt runtime details");
        sr.setIcon("dialog-information");
        sr.setScore(100.0);
        sr.setType("Developer");
        sr.setProvider("Developer Tools");
        sr.setAction("debug_info");
        results.append(sr);
    }

    if (q == ":theme" || q == "toggle theme" || q == "dark mode" || q == "light mode") {
        SearchResult sr;
        sr.setId("dev:theme");
        sr.setTitle(ThemeService::instance().isDark() ? "Switch to Light Mode" : "Switch to Dark Mode");
        sr.setSubtitle("Toggle UI color palette");
        sr.setIcon("preferences-desktop-wallpaper");
        sr.setScore(95.0);
        sr.setType("Appearance");
        sr.setProvider("Appearance");
        sr.setAction("toggle_theme");
        results.append(sr);
    }

    return results;
}

bool DeveloperCommandProvider::execute(const SearchResult &result, const QString &action)
{
    QString act = result.action();

    if (act == "open_preferences") {
        emit openPreferencesRequested();
        return true;
    }

    if (act == "reload_plugins") {
        emit reloadPluginsRequested();
        NotificationService::instance().notify("MVSpotlight", "All Lua plugins successfully reloaded");
        return true;
    }

    if (act == "toggle_theme") {
        ThemeService::instance().toggleTheme();
        return true;
    }

    if (act == "open_logs" || action == "copy_log_path") {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
        QString stateDir = QStandardPaths::writableLocation(QStandardPaths::GenericStateLocation);
#else
        QString stateDir = QDir::homePath() + "/.local/state";
#endif
        if (stateDir.isEmpty()) stateDir = QDir::homePath() + "/.local/state";
        QString logPath = stateDir + "/mvspotlight/launcher.log";

        if (action == "copy_log_path") {
            ClipboardService::instance().setText(logPath);
            NotificationService::instance().notify("MVSpotlight", "Copied log path to clipboard");
            return true;
        }

        NotificationService::instance().notify("MVSpotlight Logs", logPath);
        return true;
    }

    if (act == "debug_info") {
        QString info;
        QScreen *screen = QGuiApplication::primaryScreen();
        if (screen) {
            info = QString("Screen: %1x%2 @ %3x (Wayland)")
                    .arg(screen->geometry().width())
                    .arg(screen->geometry().height())
                    .arg(screen->devicePixelRatio());
        }
        NotificationService::instance().notify("MVSpotlight Debug", info);
        return true;
    }

    return false;
}
