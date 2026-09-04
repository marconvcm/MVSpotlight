#include "DesktopEntryService.h"
#include "UsageHistory.h"
#include "ProcessService.h"
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QStandardPaths>
#include <QRegularExpression>
#include <algorithm>
#include <QDebug>

DesktopEntryService& DesktopEntryService::instance()
{
    static DesktopEntryService s_instance;
    return s_instance;
}

DesktopEntryService::DesktopEntryService(QObject *parent)
    : QObject(parent)
{
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &DesktopEntryService::scanApplications);
    scanApplications();
}

QStringList DesktopEntryService::getApplicationDirs() const
{
    QStringList dirs;
    // Standard XDG data dirs
    QString userApps = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    if (!userApps.isEmpty()) dirs.append(userApps);

    dirs.append(QDir::homePath() + "/.local/share/flatpak/exports/share/applications");
    dirs.append("/var/lib/flatpak/exports/share/applications");
    dirs.append("/usr/local/share/applications");
    dirs.append("/usr/share/applications");

    dirs.removeDuplicates();
    return dirs;
}

void DesktopEntryService::scanApplications()
{
    m_apps.clear();
    m_appIndexById.clear();

    QStringList dirs = getApplicationDirs();
    for (const QString &dirPath : dirs) {
        QDir dir(dirPath);
        if (!dir.exists())
            continue;

        if (!m_watcher.directories().contains(dirPath)) {
            m_watcher.addPath(dirPath);
        }

        QStringList entries = dir.entryList(QStringList() << "*.desktop", QDir::Files | QDir::Readable);
        for (const QString &fileName : entries) {
            QString fullPath = dir.filePath(fileName);
            // Deduplicate by filename (user app overrides system app)
            if (!m_appIndexById.contains(fileName)) {
                parseDesktopFile(fullPath);
            }
        }
    }

    emit applicationsChanged();
}

void DesktopEntryService::parseDesktopFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QFileInfo fi(filePath);
    DesktopApp app;
    app.id = fi.fileName();
    app.filePath = filePath;

    bool inDesktopEntry = false;
    bool isApplication = false;

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;

        if (line.startsWith('[') && line.endsWith(']')) {
            inDesktopEntry = (line == "[Desktop Entry]");
            continue;
        }

        if (!inDesktopEntry)
            continue;

        int eqIdx = line.indexOf('=');
        if (eqIdx == -1)
            continue;

        QString key = line.left(eqIdx).trimmed();
        QString value = line.mid(eqIdx + 1).trimmed();

        if (key == "Type") {
            if (value == "Application") isApplication = true;
        } else if (key == "Name" && app.name.isEmpty()) {
            app.name = value;
        } else if (key == "GenericName" && app.genericName.isEmpty()) {
            app.genericName = value;
        } else if (key == "Comment" && app.comment.isEmpty()) {
            app.comment = value;
        } else if (key == "Exec") {
            app.exec = value;
        } else if (key == "Icon") {
            app.icon = value;
        } else if (key == "Terminal") {
            app.terminal = (value.compare("true", Qt::CaseInsensitive) == 0);
        } else if (key == "NoDisplay") {
            app.noDisplay = (value.compare("true", Qt::CaseInsensitive) == 0);
        } else if (key == "Keywords") {
            QStringList kw = value.split(';', Qt::SkipEmptyParts);
            for (QString &k : kw) k = k.trimmed();
            app.keywords = kw;
        } else if (key == "Categories") {
            QStringList cat = value.split(';', Qt::SkipEmptyParts);
            for (QString &c : cat) c = c.trimmed();
            app.categories = cat;
        } else if (key == "OnlyShowIn") {
            if (!value.contains("GNOME", Qt::CaseInsensitive)) {
                app.noDisplay = true;
            }
        } else if (key == "NotShowIn") {
            if (value.contains("GNOME", Qt::CaseInsensitive)) {
                app.noDisplay = true;
            }
        }
    }

    if (isApplication && !app.noDisplay && !app.name.isEmpty() && !app.exec.isEmpty()) {
        if (app.icon.isEmpty()) {
            app.icon = "application-x-executable";
        }
        int index = m_apps.size();
        m_apps.append(app);
        m_appIndexById.insert(app.id, index);
    }
}

const DesktopApp* DesktopEntryService::findApp(const QString &id) const
{
    auto it = m_appIndexById.constFind(id);
    if (it != m_appIndexById.constEnd() && it.value() < m_apps.size()) {
        return &m_apps[it.value()];
    }
    return nullptr;
}

bool DesktopEntryService::launchApp(const DesktopApp &app, const QStringList &args)
{
    UsageHistory::instance().recordLaunch("app:" + app.id);
    return launchByDesktopFile(app.filePath, args);
}

bool DesktopEntryService::launchByDesktopFile(const QString &filePath, const QStringList &args)
{
    // Try native GNOME / GIO launch first
    QStringList gioArgs;
    gioArgs << "launch" << filePath;
    if (!args.isEmpty()) {
        gioArgs.append(args);
    }

    if (ProcessService::instance().launchDetached("gio", gioArgs)) {
        return true;
    }

    // Fallback parsing exec line
    QFileInfo fi(filePath);
    const DesktopApp *app = findApp(fi.fileName());
    if (!app)
        return false;

    QString execStr = app->exec;
    // Strip %f, %F, %u, %U, etc.
    static QRegularExpression fieldCodeRegex("%[fFuUdDnNickvm]");
    execStr.replace(fieldCodeRegex, "");
    execStr = execStr.trimmed();

    QStringList parts = QProcess::splitCommand(execStr);
    if (parts.isEmpty())
        return false;

    QString binary = parts.takeFirst();
    parts.append(args);

    if (app->terminal) {
        // Find default GNOME terminal (ptyxis, gnome-terminal, kgx)
        QString terminal;
        if (!QStandardPaths::findExecutable("ptyxis").isEmpty()) {
            terminal = "ptyxis";
        } else if (!QStandardPaths::findExecutable("gnome-terminal").isEmpty()) {
            terminal = "gnome-terminal";
        } else if (!QStandardPaths::findExecutable("kgx").isEmpty()) {
            terminal = "kgx";
        } else {
            terminal = "x-terminal-emulator";
        }

        QStringList termArgs;
        termArgs << "--" << binary;
        termArgs.append(parts);
        return ProcessService::instance().launchDetached(terminal, termArgs);
    }

    return ProcessService::instance().launchDetached(binary, parts);
}

static double calculateMatchScore(const QString &target, const QString &query)
{
    if (target.isEmpty() || query.isEmpty())
        return 0.0;

    QString t = target.toLower();
    QString q = query.toLower();

    if (t == q)
        return 100.0; // Exact match

    if (t.startsWith(q))
        return 92.0 + (static_cast<double>(q.length()) / t.length()) * 5.0; // Prefix match

    // Word prefix match: e.g. "Google Chrome" matching "ch"
    static const QRegularExpression wordRegex("[\\s_\\-]+");
    QStringList words = t.split(wordRegex, Qt::SkipEmptyParts);
    for (const QString &w : words) {
        if (w.startsWith(q)) {
            return 85.0 + (static_cast<double>(q.length()) / w.length()) * 4.0;
        }
    }

    // Substring match
    int idx = t.indexOf(q);
    if (idx != -1) {
        return 70.0 - (idx * 0.5);
    }

    // Fuzzy subsequence match
    int qIdx = 0;
    int consecutive = 0;
    int maxConsecutive = 0;
    for (int i = 0; i < t.length() && qIdx < q.length(); ++i) {
        if (t[i] == q[qIdx]) {
            qIdx++;
            consecutive++;
            if (consecutive > maxConsecutive) maxConsecutive = consecutive;
        } else {
            consecutive = 0;
        }
    }

    if (qIdx == q.length()) {
        // All characters matched in order
        return 45.0 + (maxConsecutive * 5.0);
    }

    return 0.0;
}

QList<SearchResult> DesktopEntryService::search(const QString &query) const
{
    QList<SearchResult> results;
    QString q = query.trimmed();
    if (q.isEmpty()) {
        // Return top recent applications
        QStringList topRecent = UsageHistory::instance().topRecentIds(8);
        for (const QString &idWithPrefix : topRecent) {
            if (idWithPrefix.startsWith("app:")) {
                QString id = idWithPrefix.mid(4);
                const DesktopApp *app = findApp(id);
                if (app) {
                    SearchResult sr;
                    sr.setId("app:" + app->id);
                    sr.setTitle(app->name);
                    sr.setSubtitle(app->genericName.isEmpty() ? (app->comment.isEmpty() ? "Application" : app->comment) : app->genericName);
                    sr.setIcon(app->icon);
                    sr.setScore(80.0 + UsageHistory::instance().frecencyBoost("app:" + app->id));
                    sr.setType("Application");
                    sr.setProvider("Applications");
                    sr.setAction("launch");
                    sr.setMetadataValue("filePath", app->filePath);
                    results.append(sr);
                }
            }
        }
        return results;
    }

    for (const DesktopApp &app : m_apps) {
        double score = calculateMatchScore(app.name, q);

        if (!app.genericName.isEmpty()) {
            double gScore = calculateMatchScore(app.genericName, q) * 0.85;
            if (gScore > score) score = gScore;
        }

        // Check executable name match
        QFileInfo execFi(app.exec.split(' ').value(0));
        double execScore = calculateMatchScore(execFi.fileName(), q) * 0.88;
        if (execScore > score) score = execScore;

        // Check keywords
        for (const QString &kw : app.keywords) {
            double kwScore = calculateMatchScore(kw, q) * 0.75;
            if (kwScore > score) score = kwScore;
        }

        if (score > 40.0) {
            // Apply frecency boost
            score += UsageHistory::instance().frecencyBoost("app:" + app.id);

            SearchResult sr;
            sr.setId("app:" + app.id);
            sr.setTitle(app.name);
            QString sub = app.genericName;
            if (sub.isEmpty()) sub = app.comment;
            if (sub.isEmpty()) sub = "Application";
            sr.setSubtitle(sub);
            sr.setIcon(app.icon);
            sr.setScore(score);
            sr.setType("Application");
            sr.setProvider("Applications");
            sr.setAction("launch");
            sr.setMetadataValue("filePath", app.filePath);
            sr.setSecondaryActionLabel("Copy Desktop ID");
            sr.setSecondaryAction("copy_id");
            results.append(sr);
        }
    }

    std::sort(results.begin(), results.end(), [](const SearchResult &a, const SearchResult &b) {
        return a.score() > b.score();
    });

    return results;
}
