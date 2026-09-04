#include "SettingsProvider.h"
#include "../services/ProcessService.h"
#include "../services/UsageHistory.h"
#include <algorithm>

SettingsProvider::SettingsProvider(QObject *parent)
    : SearchProvider(parent)
{
    initPanels();
}

void SettingsProvider::initPanels()
{
    m_panels.append({"settings:wifi", "Wi-Fi", "wifi", "network-wireless", {"wifi", "wireless", "internet", "wlan"}});
    m_panels.append({"settings:bluetooth", "Bluetooth", "bluetooth", "bluetooth", {"bluetooth", "bt", "wireless", "devices"}});
    m_panels.append({"settings:display", "Displays", "display", "video-display", {"display", "monitor", "resolution", "screen", "refresh rate"}});
    m_panels.append({"settings:sound", "Sound", "sound", "audio-card", {"sound", "audio", "volume", "microphone", "speaker"}});
    m_panels.append({"settings:keyboard", "Keyboard", "keyboard", "input-keyboard", {"keyboard", "shortcuts", "typing", "layout"}});
    m_panels.append({"settings:mouse", "Mouse & Touchpad", "mouse", "input-mouse", {"mouse", "touchpad", "speed", "scrolling", "click"}});
    m_panels.append({"settings:appearance", "Appearance", "appearance", "preferences-desktop-wallpaper", {"appearance", "dark mode", "light mode", "wallpaper", "background", "theme"}});
    m_panels.append({"settings:power", "Power", "power", "battery", {"power", "battery", "sleep", "saver", "suspend"}});
    m_panels.append({"settings:users", "Users", "user-accounts", "system-users", {"users", "accounts", "password", "login"}});
    m_panels.append({"settings:network", "Network", "network", "network-wired", {"network", "ethernet", "vpn", "proxy", "dns"}});
    m_panels.append({"settings:privacy", "Privacy & Security", "privacy", "preferences-system-privacy", {"privacy", "security", "camera", "microphone", "location", "screen lock"}});
    m_panels.append({"settings:datetime", "Date & Time", "datetime", "preferences-system-time", {"date", "time", "clock", "timezone"}});
    m_panels.append({"settings:region", "Region & Language", "region", "preferences-desktop-locale", {"region", "language", "locale", "formats"}});
    m_panels.append({"settings:accessibility", "Accessibility", "universal-access", "preferences-desktop-accessibility", {"accessibility", "zoom", "high contrast", "screen reader"}});
    m_panels.append({"settings:notifications", "Notifications", "notifications", "preferences-system-notifications", {"notifications", "do not disturb", "alerts", "badges"}});
    m_panels.append({"settings:default-apps", "Default Applications", "default-apps", "applications-system", {"default", "apps", "browser", "mail", "music", "video"}});
}

QList<SearchResult> SettingsProvider::search(const QString &query)
{
    QList<SearchResult> results;
    QString q = query.trimmed().toLower();
    if (q.isEmpty())
        return results;

    for (const auto &panel : m_panels) {
        double score = 0.0;
        QString name = panel.name.toLower();

        if (name == q) {
            score = 96.0;
        } else if (name.startsWith(q)) {
            score = 88.0 + (static_cast<double>(q.length()) / name.length()) * 5.0;
        } else if (name.contains(q)) {
            score = 75.0;
        } else {
            for (const QString &kw : panel.keywords) {
                if (kw == q) {
                    score = std::max(score, 90.0);
                } else if (kw.startsWith(q)) {
                    score = std::max(score, 82.0);
                } else if (kw.contains(q)) {
                    score = std::max(score, 68.0);
                }
            }
        }

        if (score > 0.0) {
            score += UsageHistory::instance().frecencyBoost(panel.id);

            SearchResult sr;
            sr.setId(panel.id);
            sr.setTitle(panel.name);
            sr.setSubtitle("GNOME Settings");
            sr.setIcon(panel.icon);
            sr.setScore(score);
            sr.setType("Settings");
            sr.setProvider("GNOME Settings");
            sr.setAction("open_panel");
            sr.setMetadataValue("panel", panel.panel);
            results.append(sr);
        }
    }

    return results;
}

bool SettingsProvider::execute(const SearchResult &result, const QString &action)
{
    Q_UNUSED(action);
    QString panel = result.metadataValue("panel").toString();
    if (!panel.isEmpty()) {
        UsageHistory::instance().recordLaunch(result.id());
        return ProcessService::instance().launchDetached("gnome-control-center", {panel});
    }
    return false;
}
