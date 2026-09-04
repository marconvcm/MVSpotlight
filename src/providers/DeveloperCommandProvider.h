#pragma once

#include "SearchProvider.h"

class DeveloperCommandProvider : public SearchProvider
{
    Q_OBJECT

public:
    explicit DeveloperCommandProvider(QObject *parent = nullptr);

    QString id() const override { return "developer"; }
    QString name() const override { return "Developer Tools"; }

    QList<SearchResult> search(const QString &query) override;
    bool execute(const SearchResult &result, const QString &action = QString()) override;

signals:
    void reloadPluginsRequested();
};
