#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QFileSystemWatcher>
#include <QHash>
#include "../core/SearchResult.h"

struct DesktopApp {
    QString id; // e.g. "firefox.desktop"
    QString filePath;
    QString name;
    QString genericName;
    QString comment;
    QString exec;
    QString icon;
    QStringList keywords;
    QStringList categories;
    bool terminal{false};
    bool noDisplay{false};
};

class DesktopEntryService : public QObject
{
    Q_OBJECT

public:
    static DesktopEntryService& instance();

    void scanApplications();
    const QList<DesktopApp>& applications() const { return m_apps; }
    const DesktopApp* findApp(const QString &id) const;

    bool launchApp(const DesktopApp &app, const QStringList &args = QStringList());
    bool launchByDesktopFile(const QString &filePath, const QStringList &args = QStringList());

    QList<SearchResult> search(const QString &query) const;

signals:
    void applicationsChanged();

private:
    explicit DesktopEntryService(QObject *parent = nullptr);

    void parseDesktopFile(const QString &filePath);
    QStringList getApplicationDirs() const;

    QList<DesktopApp> m_apps;
    QHash<QString, int> m_appIndexById;
    QFileSystemWatcher m_watcher;
};
