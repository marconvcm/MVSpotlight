#include "ApplicationProvider.h"
#include "../services/DesktopEntryService.h"
#include "../services/ClipboardService.h"

ApplicationProvider::ApplicationProvider(QObject *parent)
    : SearchProvider(parent)
{
}

QList<SearchResult> ApplicationProvider::search(const QString &query)
{
    return DesktopEntryService::instance().search(query);
}

bool ApplicationProvider::execute(const SearchResult &result, const QString &action)
{
    if (action == "copy_id") {
        QString appId = result.id();
        if (appId.startsWith("app:")) appId = appId.mid(4);
        ClipboardService::instance().setText(appId);
        return true;
    }

    QString filePath = result.metadataValue("filePath").toString();
    if (!filePath.isEmpty()) {
        return DesktopEntryService::instance().launchByDesktopFile(filePath);
    }

    return false;
}
