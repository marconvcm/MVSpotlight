#pragma once

#include "SearchProvider.h"
#include <QThreadPool>
#include <atomic>

class FileProvider : public SearchProvider
{
    Q_OBJECT

public:
    explicit FileProvider(QObject *parent = nullptr);
    ~FileProvider() override;

    QString id() const override { return "files"; }
    QString name() const override { return "Files"; }
    bool isAsync() const override { return true; }

    void searchAsync(quint64 requestId, const QString &query) override;
    bool execute(const SearchResult &result, const QString &action = QString()) override;

private:
    QStringList searchDirectories() const;
    QString iconForFile(const QString &filePath, bool isDir) const;

    QThreadPool m_threadPool;
    std::atomic<quint64> m_latestRequestId{0};
};
