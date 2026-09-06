#include "SearchController.h"
#include "../providers/ApplicationProvider.h"
#include "../providers/CalculatorProvider.h"
#include "../providers/ActionProvider.h"
#include "../providers/SettingsProvider.h"
#include "../providers/DeveloperCommandProvider.h"
#include "../providers/FileProvider.h"
#include "../providers/AiProvider.h"
#include "../lua/LuaPluginManager.h"
#include "../lua/LuaSearchProvider.h"
#include "../services/UsageHistory.h"
#include "../services/ConfigService.h"
#include <algorithm>
#include <QDebug>

SearchController::SearchController(QObject *parent)
    : QObject(parent)
{
    m_debounceTimer.setSingleShot(true);
    connect(&m_debounceTimer, &QTimer::timeout, this, &SearchController::onDebounceTimeout);
}

SearchController::~SearchController()
{
}

bool SearchController::init()
{
    // Register native providers
    registerProvider(new ApplicationProvider(this));
    registerProvider(new CalculatorProvider(this));
    registerProvider(new ActionProvider(this));
    registerProvider(new SettingsProvider(this));

    DeveloperCommandProvider *devProvider = new DeveloperCommandProvider(this);
    connect(devProvider, &DeveloperCommandProvider::reloadPluginsRequested, this, &SearchController::reloadPlugins);
    connect(devProvider, &DeveloperCommandProvider::openPreferencesRequested, this, &SearchController::openPreferences);
    registerProvider(devProvider);

    registerProvider(new FileProvider(this));

    // Register AI Assistant provider
    AiProvider *aiProvider = new AiProvider(this);
    connect(aiProvider, &AiProvider::openPreferencesRequested, this, &SearchController::openPreferences);
    connect(aiProvider, &AiProvider::requestRefresh, this, [this]() {
        performSearch();
    });
    registerProvider(aiProvider);

    // Register Lua subsystem
    m_luaPluginManager = new LuaPluginManager(this);
    if (m_luaPluginManager->init()) {
        registerProvider(new LuaSearchProvider(m_luaPluginManager, this));
    }

    // Warm search with top recents
    performSearch();
    return true;
}

void SearchController::registerProvider(SearchProvider *provider)
{
    m_providers.append(provider);
    if (provider->isAsync()) {
        connect(provider, &SearchProvider::resultsReady, this, &SearchController::onAsyncResultsReady);
    }
}

void SearchController::setQuery(const QString &query)
{
    if (m_query == query)
        return;

    bool wasAi = isAiMode();
    m_query = query;
    emit queryChanged();

    if (wasAi != isAiMode()) {
        emit isAiModeChanged();
    }

    int debounceMs = ConfigService::instance().searchDebounceMs();
    if (m_query.isEmpty() || debounceMs <= 0) {
        m_debounceTimer.stop();
        performSearch();
    } else {
        m_isSearching = true;
        emit isSearchingChanged();
        m_debounceTimer.start(debounceMs);
    }
}

void SearchController::setSelectedIndex(int index)
{
    if (m_model.rowCount() == 0) {
        if (m_selectedIndex != -1) {
            m_selectedIndex = -1;
            emit selectedIndexChanged();
        }
        return;
    }
    int clamped = std::clamp(index, 0, m_model.rowCount() - 1);
    if (m_selectedIndex != clamped) {
        m_selectedIndex = clamped;
        emit selectedIndexChanged();
    }
}

void SearchController::setWindowVisible(bool visible)
{
    if (m_windowVisible != visible) {
        m_windowVisible = visible;
        emit windowVisibleChanged();
        if (visible) {
            // Re-warm search if query is empty
            if (m_query.isEmpty()) {
                performSearch();
            }
        } else {
            m_debounceTimer.stop();
            emit windowDismissed();
        }
    }
}

QVariantMap SearchController::currentResult() const
{
    return m_model.get(m_selectedIndex);
}

void SearchController::performSearch()
{
    m_debounceTimer.stop();

    m_currentRequestId++;
    quint64 reqId = m_currentRequestId;

    m_activeResults.clear();
    m_isSearching = true;
    emit isSearchingChanged();

    bool isAi = isAiMode();

    // Invoke synchronous search providers
    for (SearchProvider *provider : m_providers) {
        if (isAi && provider->id() != "ai") continue;

        if (!provider->isAsync()) {
            QList<SearchResult> res = provider->search(m_query);
            m_activeResults.append(res);
        }
    }

    // Update model with sync results first for near-instant responsiveness (<10ms)
    rankAndDeduplicateResults(m_activeResults);
    updateModelResults();

    // Trigger async search providers
    bool hasAsync = false;
    for (SearchProvider *provider : m_providers) {
        if (isAi && provider->id() != "ai") continue;

        if (provider->isAsync()) {
            hasAsync = true;
            provider->searchAsync(reqId, m_query);
        }
    }

    if (!hasAsync) {
        m_isSearching = false;
        emit isSearchingChanged();
    }
}

void SearchController::onAsyncResultsReady(quint64 requestId, const QList<SearchResult> &results)
{
    // Discard stale searches
    if (requestId != m_currentRequestId) {
        return;
    }

    m_isSearching = false;
    emit isSearchingChanged();

    if (!results.isEmpty()) {
        m_activeResults.append(results);
        rankAndDeduplicateResults(m_activeResults);
        updateModelResults();
    }
}

void SearchController::rankAndDeduplicateResults(QList<SearchResult> &results)
{
    // Deduplicate by ID
    QHash<QString, SearchResult> unique;
    for (const SearchResult &item : results) {
        if (!item.isValid()) continue;
        auto it = unique.find(item.id());
        if (it == unique.end() || item.score() > it.value().score()) {
            unique.insert(item.id(), item);
        }
    }

    results = unique.values();

    // Sort descending by score
    std::sort(results.begin(), results.end(), [](const SearchResult &a, const SearchResult &b) {
        return a.score() > b.score();
    });
}

void SearchController::updateModelResults()
{
    m_model.setResults(m_activeResults);
    m_selectedIndex = m_activeResults.isEmpty() ? -1 : 0;
    emit selectedIndexChanged();
    emit resultCountChanged();
}

void SearchController::selectNext()
{
    flushSearch();
    if (m_model.rowCount() > 0) {
        setSelectedIndex((m_selectedIndex + 1) % m_model.rowCount());
    }
}

void SearchController::selectPrevious()
{
    flushSearch();
    if (m_model.rowCount() > 0) {
        setSelectedIndex((m_selectedIndex - 1 + m_model.rowCount()) % m_model.rowCount());
    }
}

SearchProvider* SearchController::findProviderForResult(const SearchResult &result) const
{
    const QString prov = result.provider();
    const QString id = result.id();

    // 1. Direct match on provider id or name
    for (SearchProvider *provider : m_providers) {
        if (provider->id().compare(prov, Qt::CaseInsensitive) == 0 ||
            provider->name().compare(prov, Qt::CaseInsensitive) == 0) {
            return provider;
        }
    }

    // 2. Prefix mapping based on result ID
    if (id.startsWith("settings:")) {
        for (SearchProvider *p : m_providers) {
            if (p->id() == "settings") return p;
        }
    } else if (id.startsWith("app:")) {
        for (SearchProvider *p : m_providers) {
            if (p->id() == "apps") return p;
        }
    } else if (id.startsWith("action:")) {
        for (SearchProvider *p : m_providers) {
            if (p->id() == "actions") return p;
        }
    } else if (id.startsWith("dev:")) {
        for (SearchProvider *p : m_providers) {
            if (p->id() == "developer") return p;
        }
    } else if (id.startsWith("file:")) {
        for (SearchProvider *p : m_providers) {
            if (p->id() == "files") return p;
        }
    } else if (id.startsWith("calc:")) {
        for (SearchProvider *p : m_providers) {
            if (p->id() == "calc") return p;
        }
    } else if (id.startsWith("ai:")) {
        for (SearchProvider *p : m_providers) {
            if (p->id() == "ai") return p;
        }
    }

    // 3. Lua plugin metadata check
    if (!result.metadataValue("pluginId").toString().isEmpty()) {
        for (SearchProvider *p : m_providers) {
            if (p->id() == "lua_plugins") return p;
        }
    }

    return nullptr;
}

void SearchController::executeIndex(int index)
{
    flushSearch();
    const SearchResult *res = m_model.resultAt(index);
    if (!res) return;

    bool keepOpen = (res->action() == "ask_ai" || res->action() == "none");

    if (res->action() == "ask_ai") {
        QString prompt = res->metadataValue("prompt").toString();
        if (!m_query.trimmed().startsWith('>')) {
            setQuery("> " + prompt);
        }
    }

    SearchProvider *provider = findProviderForResult(*res);
    if (provider && provider->execute(*res)) {
        emit resultLaunched();
        if (!keepOpen) {
            hideWindow();
        }
        return;
    }

    // Fallback: try remaining providers
    for (SearchProvider *p : m_providers) {
        if (p != provider && p->execute(*res)) {
            emit resultLaunched();
            if (!keepOpen) {
                hideWindow();
            }
            return;
        }
    }
}

void SearchController::executeSecondaryIndex(int index)
{
    flushSearch();
    const SearchResult *res = m_model.resultAt(index);
    if (!res) return;

    QString secAction = res->secondaryAction();
    QString action = secAction.isEmpty() ? QStringLiteral("secondary") : secAction;
    bool keepOpen = (action == "ask_ai" || action == "none");

    SearchProvider *provider = findProviderForResult(*res);
    if (provider && provider->execute(*res, action)) {
        emit resultLaunched();
        if (!keepOpen) {
            hideWindow();
        }
        return;
    }

    for (SearchProvider *p : m_providers) {
        if (p != provider && p->execute(*res, action)) {
            emit resultLaunched();
            if (!keepOpen) {
                hideWindow();
            }
            return;
        }
    }
}

void SearchController::executeSelected()
{
    executeIndex(m_selectedIndex);
}

void SearchController::executeSecondarySelected()
{
    executeSecondaryIndex(m_selectedIndex);
}

void SearchController::showWindow()
{
    setWindowVisible(true);
}

void SearchController::hideWindow()
{
    setWindowVisible(false);
}

void SearchController::toggleWindow()
{
    setWindowVisible(!m_windowVisible);
}

void SearchController::reloadPlugins()
{
    if (m_luaPluginManager) {
        m_luaPluginManager->reloadAll();
        performSearch();
    }
}

void SearchController::openPreferences()
{
    emit openPreferencesRequested();
}

void SearchController::onDebounceTimeout()
{
    performSearch();
}

void SearchController::flushSearch()
{
    if (m_debounceTimer.isActive()) {
        m_debounceTimer.stop();
        performSearch();
    }
}
