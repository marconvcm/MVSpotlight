#pragma once

#include "SearchProvider.h"

class ApplicationProvider : public SearchProvider
{
    Q_OBJECT

public:
    explicit ApplicationProvider(QObject *parent = nullptr);

    QString id() const override { return "applications"; }
    QString name() const override { return "Applications"; }

    QList<SearchResult> search(const QString &query) override;
    bool execute(const SearchResult &result, const QString &action = QString()) override;
};
