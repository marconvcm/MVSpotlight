#pragma once

#include "../providers/SearchProvider.h"

class LuaPluginManager;

class LuaSearchProvider : public SearchProvider
{
    Q_OBJECT

public:
    explicit LuaSearchProvider(LuaPluginManager *manager, QObject *parent = nullptr);

    QString id() const override { return "lua_plugins"; }
    QString name() const override { return "Lua Plugins"; }

    QList<SearchResult> search(const QString &query) override;
    bool execute(const SearchResult &result, const QString &action = QString()) override;

private:
    LuaPluginManager *m_manager;
};
