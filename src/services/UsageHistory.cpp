#include "UsageHistory.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <algorithm>
#include <cmath>

UsageHistory& UsageHistory::instance()
{
    static UsageHistory s_instance;
    return s_instance;
}

UsageHistory::UsageHistory(QObject *parent)
    : QObject(parent)
{
    load();
}

UsageHistory::~UsageHistory()
{
    save();
}

QString UsageHistory::storageFilePath() const
{
    QString stateDir = QStandardPaths::writableLocation(QStandardPaths::GenericStateLocation);
    if (stateDir.isEmpty())
        stateDir = QDir::homePath() + "/.local/state";

    QString newDir = stateDir + "/mvspotlight";
    QString newPath = newDir + "/history.json";
    QString oldPath = stateDir + "/spotlight-qt/history.json";

    QDir().mkpath(newDir);

    // Auto-migrate legacy history if it exists and new history doesn't
    if (!QFile::exists(newPath) && QFile::exists(oldPath)) {
        QFile::copy(oldPath, newPath);
    }

    return newPath;
}

void UsageHistory::load()
{
    QFile file(storageFilePath());
    if (!file.open(QIODevice::ReadOnly))
        return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject())
        return;

    QJsonObject root = doc.object();
    QJsonArray items = root["items"].toArray();

    m_entries.clear();
    for (const QJsonValue &val : items) {
        QJsonObject obj = val.toObject();
        HistoryEntry entry;
        entry.id = obj["id"].toString();
        entry.launchCount = obj["count"].toInt();
        entry.lastLaunched = QDateTime::fromString(obj["last"].toString(), Qt::ISODate);
        if (!entry.id.isEmpty()) {
            m_entries.insert(entry.id, entry);
        }
    }
}

void UsageHistory::save()
{
    QJsonArray items;
    for (auto it = m_entries.constBegin(); it != m_entries.constEnd(); ++it) {
        QJsonObject obj;
        obj["id"] = it.value().id;
        obj["count"] = it.value().launchCount;
        obj["last"] = it.value().lastLaunched.toString(Qt::ISODate);
        items.append(obj);
    }

    QJsonObject root;
    root["items"] = items;

    QFile file(storageFilePath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    }
}

void UsageHistory::clear()
{
    m_entries.clear();
    save();
}

void UsageHistory::recordLaunch(const QString &id)
{
    if (id.isEmpty())
        return;

    auto it = m_entries.find(id);
    if (it != m_entries.end()) {
        it->launchCount += 1;
        it->lastLaunched = QDateTime::currentDateTime();
    } else {
        HistoryEntry entry;
        entry.id = id;
        entry.launchCount = 1;
        entry.lastLaunched = QDateTime::currentDateTime();
        m_entries.insert(id, entry);
    }
    save();
}

double UsageHistory::frecencyBoost(const QString &id) const
{
    auto it = m_entries.constFind(id);
    if (it == m_entries.constEnd())
        return 0.0;

    const HistoryEntry &entry = it.value();
    qint64 secondsAgo = entry.lastLaunched.secsTo(QDateTime::currentDateTime());
    if (secondsAgo < 0) secondsAgo = 0;

    // Recency weight: within 1 hour = 1.0, 1 day = 0.8, 1 week = 0.5, 1 month = 0.2
    double recencyFactor = 0.1;
    if (secondsAgo < 3600) {
        recencyFactor = 1.0;
    } else if (secondsAgo < 86400) {
        recencyFactor = 0.8;
    } else if (secondsAgo < 86400 * 7) {
        recencyFactor = 0.5;
    } else if (secondsAgo < 86400 * 30) {
        recencyFactor = 0.25;
    }

    // Frequency factor: logarithmic scaling up to 10 points
    double frequencyFactor = std::min(10.0, std::log2(1.0 + entry.launchCount) * 3.0);

    // Total boost up to ~25 points
    return (frequencyFactor * recencyFactor) * 2.0;
}

QStringList UsageHistory::topRecentIds(int limit) const
{
    QList<HistoryEntry> sorted = m_entries.values();
    std::sort(sorted.begin(), sorted.end(), [](const HistoryEntry &a, const HistoryEntry &b) {
        if (a.lastLaunched != b.lastLaunched)
            return a.lastLaunched > b.lastLaunched;
        return a.launchCount > b.launchCount;
    });

    QStringList result;
    for (int i = 0; i < std::min(limit, static_cast<int>(sorted.size())); ++i) {
        result.append(sorted[i].id);
    }
    return result;
}
