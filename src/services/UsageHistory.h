#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QHash>
#include <QJsonObject>
#include <QJsonDocument>

struct HistoryEntry {
    QString id;
    int launchCount{0};
    QDateTime lastLaunched;
};

class UsageHistory : public QObject
{
    Q_OBJECT

public:
    static UsageHistory& instance();

    void recordLaunch(const QString &id);
    double frecencyBoost(const QString &id) const;
    QStringList topRecentIds(int limit = 10) const;

    void load();
    void save();
    Q_INVOKABLE void clear();
    Q_INVOKABLE int entryCount() const { return m_entries.size(); }

private:
    explicit UsageHistory(QObject *parent = nullptr);
    ~UsageHistory() override;

    QString storageFilePath() const;

    QHash<QString, HistoryEntry> m_entries;
};
