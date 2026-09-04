#include "SystemActions.h"
#include "ProcessService.h"
#include "UsageHistory.h"
#include <QDir>
#include <QStandardPaths>
#include <algorithm>

SystemActions& SystemActions::instance()
{
    static SystemActions s_instance;
    return s_instance;
}

SystemActions::SystemActions(QObject *parent)
    : QObject(parent)
{
    initActions();
}

void SystemActions::initActions()
{
    m_actions.clear();

    m_actions.append({
        "action:lock",
        "Lock Screen",
        "Lock current session",
        "system-lock-screen",
        {"lock", "screen", "session"},
        "loginctl",
        {"lock-session"}
    });

    m_actions.append({
        "action:logout",
        "Log Out",
        "End session",
        "system-log-out",
        {"logout", "log out", "exit", "sign out"},
        "gnome-session-quit",
        {"--logout"}
    });

    m_actions.append({
        "action:suspend",
        "Suspend",
        "Put computer to sleep",
        "system-suspend",
        {"suspend", "sleep", "standby"},
        "systemctl",
        {"suspend"}
    });

    m_actions.append({
        "action:restart",
        "Restart",
        "Reboot the system",
        "system-reboot",
        {"restart", "reboot"},
        "systemctl",
        {"reboot"}
    });

    m_actions.append({
        "action:shutdown",
        "Shut Down",
        "Turn off the computer",
        "system-shutdown",
        {"shutdown", "shut down", "poweroff", "turn off"},
        "systemctl",
        {"poweroff"}
    });

    QString terminal = "x-terminal-emulator";
    if (!QStandardPaths::findExecutable("ptyxis").isEmpty()) terminal = "ptyxis";
    else if (!QStandardPaths::findExecutable("gnome-terminal").isEmpty()) terminal = "gnome-terminal";
    else if (!QStandardPaths::findExecutable("kgx").isEmpty()) terminal = "kgx";

    m_actions.append({
        "action:terminal",
        "Open Terminal",
        "Launch command line terminal",
        "utilities-terminal",
        {"terminal", "console", "shell", "bash", "zsh", "ptyxis"},
        terminal,
        {}
    });

    QString downloadsPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (downloadsPath.isEmpty()) downloadsPath = QDir::homePath() + "/Downloads";
    m_actions.append({
        "action:downloads",
        "Open Downloads",
        downloadsPath,
        "folder-download",
        {"downloads", "download", "folder"},
        "gio",
        {"open", downloadsPath}
    });

    QString docsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (docsPath.isEmpty()) docsPath = QDir::homePath() + "/Documents";
    m_actions.append({
        "action:documents",
        "Open Documents",
        docsPath,
        "folder-documents",
        {"documents", "docs", "folder"},
        "gio",
        {"open", docsPath}
    });

    m_actions.append({
        "action:home",
        "Open Home Directory",
        QDir::homePath(),
        "user-home",
        {"home", "user", "folder", "files"},
        "gio",
        {"open", QDir::homePath()}
    });

    m_actions.append({
        "action:empty-trash",
        "Empty Trash",
        "Permanently delete trashed items",
        "user-trash",
        {"trash", "empty", "recycle", "clean"},
        "gio",
        {"trash", "--empty"}
    });
}

QList<SearchResult> SystemActions::search(const QString &query) const
{
    QList<SearchResult> results;
    QString q = query.trimmed().toLower();
    if (q.isEmpty())
        return results;

    for (const auto &item : m_actions) {
        double score = 0.0;
        QString t = item.title.toLower();

        if (t == q) {
            score = 98.0;
        } else if (t.startsWith(q)) {
            score = 90.0 + (static_cast<double>(q.length()) / t.length()) * 5.0;
        } else if (t.contains(q)) {
            score = 75.0;
        } else {
            for (const QString &kw : item.keywords) {
                if (kw == q) {
                    score = std::max(score, 92.0);
                } else if (kw.startsWith(q)) {
                    score = std::max(score, 85.0);
                } else if (kw.contains(q)) {
                    score = std::max(score, 70.0);
                }
            }
        }

        if (score > 0.0) {
            score += UsageHistory::instance().frecencyBoost(item.id);
            SearchResult sr;
            sr.setId(item.id);
            sr.setTitle(item.title);
            sr.setSubtitle(item.subtitle);
            sr.setIcon(item.icon);
            sr.setScore(score);
            sr.setType("Action");
            sr.setProvider("System Actions");
            sr.setAction("execute");
            sr.setMetadataValue("command", item.command);
            sr.setMetadataValue("args", item.args);
            results.append(sr);
        }
    }

    return results;
}

bool SystemActions::execute(const QString &actionId)
{
    for (const auto &item : m_actions) {
        if (item.id == actionId) {
            UsageHistory::instance().recordLaunch(item.id);
            return ProcessService::instance().launchDetached(item.command, item.args);
        }
    }
    return false;
}
