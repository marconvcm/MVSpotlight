#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <atomic>
#include "SearchResultModel.h"
#include "../providers/SearchProvider.h"

class LuaPluginManager;

class SearchController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
    Q_PROPERTY(int selectedIndex READ selectedIndex WRITE setSelectedIndex NOTIFY selectedIndexChanged)
    Q_PROPERTY(bool isSearching READ isSearching NOTIFY isSearchingChanged)
    Q_PROPERTY(SearchResultModel* model READ model CONSTANT)
    Q_PROPERTY(int resultCount READ resultCount NOTIFY resultCountChanged)
    Q_PROPERTY(bool windowVisible READ isWindowVisible WRITE setWindowVisible NOTIFY windowVisibleChanged)
    Q_PROPERTY(QVariantMap currentResult READ currentResult NOTIFY selectedIndexChanged)

public:
    explicit SearchController(QObject *parent = nullptr);
    ~SearchController() override;

    bool init();

    QString query() const { return m_query; }
    void setQuery(const QString &query);

    int selectedIndex() const { return m_selectedIndex; }
    void setSelectedIndex(int index);

    bool isSearching() const { return m_isSearching; }
    SearchResultModel* model() { return &m_model; }
    int resultCount() const { return m_model.rowCount(); }
    bool isWindowVisible() const { return m_windowVisible; }
    void setWindowVisible(bool visible);

    QVariantMap currentResult() const;

    Q_INVOKABLE void executeSelected();
    Q_INVOKABLE void executeSecondarySelected();
    Q_INVOKABLE void selectNext();
    Q_INVOKABLE void selectPrevious();
    Q_INVOKABLE void executeIndex(int index);
    Q_INVOKABLE void executeSecondaryIndex(int index);

    Q_INVOKABLE void showWindow();
    Q_INVOKABLE void hideWindow();
    Q_INVOKABLE void toggleWindow();
    Q_INVOKABLE void reloadPlugins();

    LuaPluginManager* pluginManager() { return m_luaPluginManager; }

signals:
    void queryChanged();
    void selectedIndexChanged();
    void isSearchingChanged();
    void resultCountChanged();
    void windowVisibleChanged();
    void resultLaunched();
    void windowDismissed();

private slots:
    void onAsyncResultsReady(quint64 requestId, const QList<SearchResult> &results);

private:
    void registerProvider(SearchProvider *provider);
    void performSearch();
    void updateModelResults();
    void rankAndDeduplicateResults(QList<SearchResult> &results);

    QString m_query;
    int m_selectedIndex{0};
    bool m_isSearching{false};
    bool m_windowVisible{false};

    quint64 m_currentRequestId{0};
    QList<SearchProvider*> m_providers;
    SearchResultModel m_model;
    LuaPluginManager *m_luaPluginManager{nullptr};

    // Stored sync & async results for the active query
    QList<SearchResult> m_activeResults;
};
