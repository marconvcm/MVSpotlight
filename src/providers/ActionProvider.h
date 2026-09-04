#pragma once

#include "SearchProvider.h"

class ActionProvider : public SearchProvider
{
    Q_OBJECT

public:
    explicit ActionProvider(QObject *parent = nullptr);

    QString id() const override { return "actions"; }
    QString name() const override { return "Quick Actions"; }

    QList<SearchResult> search(const QString &query) override;
    bool execute(const SearchResult &result, const QString &action = QString()) override;
};
