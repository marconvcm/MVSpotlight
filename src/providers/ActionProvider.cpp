#include "ActionProvider.h"
#include "../services/SystemActions.h"

ActionProvider::ActionProvider(QObject *parent)
    : SearchProvider(parent)
{
}

QList<SearchResult> ActionProvider::search(const QString &query)
{
    return SystemActions::instance().search(query);
}

bool ActionProvider::execute(const SearchResult &result, const QString &action)
{
    Q_UNUSED(action);
    return SystemActions::instance().execute(result.id());
}
