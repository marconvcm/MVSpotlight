#pragma once

#include <QObject>
#include <QList>
#include <QHash>
#include <QStringList>
#include <QFileSystemWatcher>
#include "LuaPlugin.h"
#include "LuaEngine.h"
#include "../core/SearchResult.h"

class LuaApi;

class LuaPluginManager : public QObject
{
    Q_OBJECT

public:
    static constexpr int CURRENT_API_VERSION = 1;

    explicit LuaPluginManager(QObject *parent = nullptr);
    ~LuaPluginManager() override;

    bool init();
    void discoverAndLoadPlugins();
    void reloadAll();

    const QList<LuaPlugin>& plugins() const { return m_plugins; }
    LuaPlugin* findPlugin(const QString &id);

    LuaPlugin* currentLoadingPlugin() const { return m_currentLoadingPlugin; }
    void setCurrentLoadingPlugin(LuaPlugin *p) { m_currentLoadingPlugin = p; }

    LuaPlugin* currentExecutingPlugin() const { return m_currentExecutingPlugin; }
    void setCurrentExecutingPlugin(LuaPlugin *p) { m_currentExecutingPlugin = p; }

    LuaEngine* engine() { return &m_engine; }

    // Search and execution
    QList<SearchResult> searchAll(const QString &query);
    bool executeCommand(const QString &pluginId, const QString &commandId);
    bool executeProvider(const QString &pluginId, const QString &providerId, const SearchResult &result);

signals:
    void pluginsReloaded();
    void pluginError(const QString &pluginId, const QString &error);

private slots:
    void onDirectoryChanged(const QString &path);

private:
    QStringList getPluginDirectories() const;
    bool loadPluginFromDir(const QString &dirPath);
    bool loadManifest(const QString &manifestPath, LuaPlugin &plugin);

    LuaEngine m_engine;
    LuaApi *m_api{nullptr};
    QList<LuaPlugin> m_plugins;
    QHash<QString, int> m_pluginIndexById;

    LuaPlugin *m_currentLoadingPlugin{nullptr};
    LuaPlugin *m_currentExecutingPlugin{nullptr};

    QFileSystemWatcher m_watcher;
};
