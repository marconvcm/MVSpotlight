#include "LuaSearchProvider.h"
#include "LuaPluginManager.h"
#include "../services/ClipboardService.h"

LuaSearchProvider::LuaSearchProvider(LuaPluginManager *manager, QObject *parent)
    : SearchProvider(parent)
    , m_manager(manager)
{
}

QList<SearchResult> LuaSearchProvider::search(const QString &query)
{
    if (!m_manager)
        return {};
    return m_manager->searchAll(query);
}

bool LuaSearchProvider::execute(const SearchResult &result, const QString &action)
{
    if (!m_manager)
        return false;

    if (action == "secondary") {
        if (!result.secondaryAction().isEmpty()) {
            ClipboardService::instance().setText(result.secondaryAction());
            return true;
        }
    }

    QString act = result.action();
    QString pluginId = result.metadataValue("pluginId").toString();

    if (act == "execute_command") {
        QString cmdId = result.metadataValue("commandId").toString();
        return m_manager->executeCommand(pluginId, cmdId);
    } else if (act == "execute_provider") {
        QString provId = result.metadataValue("providerId").toString();
        return m_manager->executeProvider(pluginId, provId, result);
    }

    return false;
}
