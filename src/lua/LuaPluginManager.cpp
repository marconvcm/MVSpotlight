#include "LuaPluginManager.h"
#include "LuaApi.h"
#include "../services/UsageHistory.h"
#include "../services/ConfigService.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QCoreApplication>
#include <algorithm>
#include <QDebug>

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

LuaPluginManager::LuaPluginManager(QObject *parent)
    : QObject(parent)
{
    m_api = new LuaApi(this, this);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &LuaPluginManager::onDirectoryChanged);
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &LuaPluginManager::onDirectoryChanged);
}

LuaPluginManager::~LuaPluginManager()
{
}

bool LuaPluginManager::init()
{
    if (!m_engine.init())
        return false;

    m_api->registerApi(m_engine.state());
    discoverAndLoadPlugins();
    return true;
}

void LuaPluginManager::onDirectoryChanged(const QString &path)
{
    Q_UNUSED(path);
    qDebug() << "Plugin directory change detected. Reloading plugins...";
    reloadAll();
}

QStringList LuaPluginManager::getPluginDirectories() const
{
    QStringList dirs;

    // User plugins
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    if (dataDir.isEmpty()) dataDir = QDir::homePath() + "/.local/share";
    dirs.append(dataDir + "/mvspotlight/plugins");
    dirs.append(dataDir + "/spotlight-qt/plugins");

    // Bundled / relative plugins
    dirs.append(QCoreApplication::applicationDirPath() + "/plugins/examples");
    dirs.append(QCoreApplication::applicationDirPath() + "/../plugins/examples");
    dirs.append(QDir::currentPath() + "/plugins/examples");

    // System plugins
    dirs.append("/usr/share/mvspotlight/plugins");
    dirs.append("/usr/share/spotlight-qt/plugins");

    dirs.removeDuplicates();
    return dirs;
}

void LuaPluginManager::reloadAll()
{
    qDebug() << "Reloading all Lua plugins...";
    m_plugins.clear();
    m_pluginIndexById.clear();
    m_currentLoadingPlugin = nullptr;
    m_currentExecutingPlugin = nullptr;

    m_engine.cleanup();
    m_engine.init();
    m_api->registerApi(m_engine.state());

    discoverAndLoadPlugins();
    emit pluginsReloaded();
}

void LuaPluginManager::discoverAndLoadPlugins()
{
    QStringList baseDirs = getPluginDirectories();

    for (const QString &baseDir : baseDirs) {
        QDir dir(baseDir);
        if (!dir.exists())
            continue;

        if (!m_watcher.directories().contains(baseDir)) {
            m_watcher.addPath(baseDir);
        }

        QStringList subDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable);
        for (const QString &sub : subDirs) {
            QString pluginDirPath = dir.filePath(sub);
            loadPluginFromDir(pluginDirPath);
        }
    }
}

bool LuaPluginManager::loadManifest(const QString &manifestPath, LuaPlugin &plugin)
{
    QFile file(manifestPath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject())
        return false;

    QJsonObject obj = doc.object();
    plugin.setId(obj["id"].toString());
    plugin.setName(obj["name"].toString());
    plugin.setVersion(obj["version"].toString("1.0.0"));
    plugin.setDescription(obj["description"].toString());
    plugin.setAuthor(obj["author"].toString());
    plugin.setEntry(obj["entry"].toString("plugin.lua"));
    plugin.setIcon(obj["icon"].toString());
    plugin.setMinimumApiVersion(obj["minimumApiVersion"].toInt(1));

    QStringList perms;
    QJsonArray permArr = obj["permissions"].toArray();
    for (const QJsonValue &pv : permArr) {
        perms.append(pv.toString());
    }
    plugin.setPermissions(LuaPermissions(perms));

    if (obj.contains("settings") && obj["settings"].isArray()) {
        plugin.setSettingsSchema(obj["settings"].toArray().toVariantList());
    }

    if (plugin.minimumApiVersion() > CURRENT_API_VERSION) {
        plugin.setStatus(LuaPlugin::Status::Error);
        plugin.setErrorMessage(QString("Incompatible API version: requires %1, launcher provides %2")
                               .arg(plugin.minimumApiVersion()).arg(CURRENT_API_VERSION));
        return false;
    }

    return !plugin.id().isEmpty();
}

bool LuaPluginManager::loadPluginFromDir(const QString &dirPath)
{
    QString manifestPath = dirPath + "/manifest.json";
    if (!QFile::exists(manifestPath))
        return false;

    LuaPlugin plugin;
    plugin.setDirectory(dirPath);

    if (!loadManifest(manifestPath, plugin)) {
        if (!plugin.id().isEmpty()) {
            m_plugins.append(plugin);
            m_pluginIndexById.insert(plugin.id(), m_plugins.size() - 1);
        }
        return false;
    }

    // Deduplicate
    if (m_pluginIndexById.contains(plugin.id())) {
        return false;
    }

    if (!m_watcher.directories().contains(dirPath)) {
        m_watcher.addPath(dirPath);
    }

    // Check if user has disabled this plugin in settings
    bool isEnabled = ConfigService::instance().isPluginEnabled(plugin.id());
    if (!isEnabled) {
        plugin.setStatus(LuaPlugin::Status::Disabled);
        int idx = m_plugins.size();
        m_plugins.append(plugin);
        m_pluginIndexById.insert(plugin.id(), idx);
        return true;
    }

    QString entryPath = dirPath + "/" + plugin.entry();
    if (!QFile::exists(entryPath)) {
        plugin.setStatus(LuaPlugin::Status::Error);
        plugin.setErrorMessage("Entry file not found: " + plugin.entry());
        int idx = m_plugins.size();
        m_plugins.append(plugin);
        m_pluginIndexById.insert(plugin.id(), idx);
        return false;
    }

    if (!m_watcher.files().contains(entryPath)) {
        m_watcher.addPath(entryPath);
    }

    int pluginIndex = m_plugins.size();
    plugin.setStatus(LuaPlugin::Status::Enabled);
    m_plugins.append(plugin);
    m_pluginIndexById.insert(plugin.id(), pluginIndex);

    // Load Lua script
    m_currentLoadingPlugin = &m_plugins[pluginIndex];
    m_currentExecutingPlugin = &m_plugins[pluginIndex];

    QString errorMsg;
    lua_State *L = m_engine.state();

    // Check if script returned a table with search / execute
    lua_pushcfunction(L, LuaEngine::errorHandler);
    int errIdx = lua_gettop(L);

    QFile entryFile(entryPath);
    if (entryFile.open(QIODevice::ReadOnly)) {
        QByteArray code = entryFile.readAll();
        entryFile.close();

        if (luaL_loadbuffer(L, code.constData(), code.size(), entryPath.toUtf8().constData()) == LUA_OK) {
            m_engine.enableWatchdog();
            int res = lua_pcall(L, 0, 1, errIdx);
            m_engine.disableWatchdog();

            if (res == LUA_OK) {
                // If the file returned a table, support table-style registration
                if (lua_istable(L, -1)) {
                    lua_getfield(L, -1, "search");
                    bool hasSearch = lua_isfunction(L, -1);
                    lua_pop(L, 1);

                    if (hasSearch) {
                        LuaProviderDef prov;
                        prov.id = m_plugins[pluginIndex].id();
                        lua_getfield(L, -1, "search");
                        prov.searchRef = luaL_ref(L, LUA_REGISTRYINDEX);
                        lua_getfield(L, -1, "execute");
                        if (lua_isfunction(L, -1)) {
                            prov.executeRef = luaL_ref(L, LUA_REGISTRYINDEX);
                        } else {
                            lua_pop(L, 1);
                        }
                        m_plugins[pluginIndex].addProvider(prov);
                    }
                }
                lua_pop(L, 1); // pop return value or table
            } else {
                errorMsg = QString::fromUtf8(lua_tostring(L, -1));
                lua_pop(L, 1);
            }
        } else {
            errorMsg = QString::fromUtf8(lua_tostring(L, -1));
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1); // pop error handler

    m_currentLoadingPlugin = nullptr;
    m_currentExecutingPlugin = nullptr;

    if (!errorMsg.isEmpty()) {
        m_plugins[pluginIndex].recordFailure(errorMsg);
        LuaApi::logToFile(m_plugins[pluginIndex].id(), "Load error: " + errorMsg);
        emit pluginError(m_plugins[pluginIndex].id(), errorMsg);
        return false;
    }

    qDebug() << "Successfully loaded Lua plugin:" << m_plugins[pluginIndex].name()
             << "with" << m_plugins[pluginIndex].commands().size() << "commands and"
             << m_plugins[pluginIndex].providers().size() << "providers";
    return true;
}

LuaPlugin* LuaPluginManager::findPlugin(const QString &id)
{
    auto it = m_pluginIndexById.find(id);
    if (it != m_pluginIndexById.end() && it.value() < m_plugins.size()) {
        return &m_plugins[it.value()];
    }
    return nullptr;
}

static double matchCommand(const QString &title, const QStringList &keywords, const QString &query)
{
    QString q = query.trimmed().toLower();
    if (q.isEmpty()) return 0.0;

    QString t = title.toLower();
    if (t == q) return 100.0;
    if (t.startsWith(q)) return 92.0 + (static_cast<double>(q.length()) / t.length()) * 5.0;
    if (t.contains(q)) return 76.0;

    for (const QString &kw : keywords) {
        QString k = kw.toLower();
        if (k == q) return 96.0;
        if (k.startsWith(q)) return 88.0;
        if (k.contains(q)) return 72.0;
    }
    return 0.0;
}

QList<SearchResult> LuaPluginManager::searchAll(const QString &query)
{
    QList<SearchResult> results;
    lua_State *L = m_engine.state();
    if (!L) return results;

    for (LuaPlugin &plugin : m_plugins) {
        if (plugin.status() != LuaPlugin::Status::Enabled)
            continue;

        m_currentExecutingPlugin = &plugin;

        // 1. Check registered commands
        for (const LuaCommandDef &cmd : plugin.commands()) {
            double score = matchCommand(cmd.title, cmd.keywords, query);
            if (score > 0.0) {
                score += UsageHistory::instance().frecencyBoost(cmd.id);
                SearchResult sr;
                sr.setId(cmd.id);
                sr.setTitle(cmd.title);
                sr.setSubtitle(cmd.subtitle.isEmpty() ? ("Lua Plugin · " + plugin.name()) : cmd.subtitle);
                sr.setIcon(cmd.icon.isEmpty() ? plugin.icon() : cmd.icon);
                sr.setScore(score);
                sr.setType("Command");
                sr.setProvider(plugin.name());
                sr.setAction("execute_command");
                sr.setMetadataValue("pluginId", plugin.id());
                sr.setMetadataValue("commandId", cmd.id);
                results.append(sr);
            }
        }

        // 2. Check registered search providers
        for (const LuaProviderDef &prov : plugin.providers()) {
            if (prov.searchRef == -1)
                continue;

            lua_pushcfunction(L, LuaEngine::errorHandler);
            int errIdx = lua_gettop(L);

            lua_rawgeti(L, LUA_REGISTRYINDEX, prov.searchRef);
            lua_pushstring(L, query.toUtf8().constData());

            m_engine.enableWatchdog(500000); // 100ms soft limit
            int res = lua_pcall(L, 1, 1, errIdx);
            m_engine.disableWatchdog();

            if (res == LUA_OK) {
                if (lua_istable(L, -1)) {
                    QVariantList list = LuaEngine::toVariantList(L, -1);
                    for (const QVariant &itemVar : list) {
                        QVariantMap itemMap = itemVar.toMap();
                        QString title = itemMap.value("title").toString();
                        if (title.isEmpty()) continue;

                        SearchResult sr;
                        QString rId = itemMap.value("id").toString();
                        if (rId.isEmpty()) {
                            rId = plugin.id() + ":" + QString::number(qHash(title));
                        }
                        sr.setId(rId);
                        sr.setTitle(title);
                        sr.setSubtitle(itemMap.value("subtitle", "Lua Plugin · " + plugin.name()).toString());
                        QString ic = itemMap.value("icon").toString();
                        sr.setIcon(ic.isEmpty() ? plugin.icon() : ic);
                        double sc = itemMap.value("score", 75.0).toDouble();
                        sr.setScore(sc + UsageHistory::instance().frecencyBoost(sr.id()));
                        sr.setType(itemMap.value("type", "Plugin").toString());
                        sr.setProvider(plugin.name());
                        sr.setAction("execute_provider");
                        sr.setMetadataValue("pluginId", plugin.id());
                        sr.setMetadataValue("providerId", prov.id);
                        sr.setMetadataValue("data", itemMap.value("data"));
                        if (itemMap.contains("secondaryAction")) {
                            sr.setSecondaryAction(itemMap.value("secondaryAction").toString());
                            sr.setSecondaryActionLabel(itemMap.value("secondaryActionLabel").toString());
                        }
                        results.append(sr);
                    }
                }
                lua_pop(L, 1); // pop result table
            } else {
                QString err = QString::fromUtf8(lua_tostring(L, -1));
                lua_pop(L, 1);
                plugin.recordFailure(err);
                LuaApi::logToFile(plugin.id(), "Search error: " + err);
            }

            lua_pop(L, 1); // pop error handler
        }

        m_currentExecutingPlugin = nullptr;
    }

    return results;
}

bool LuaPluginManager::executeCommand(const QString &pluginId, const QString &commandId)
{
    LuaPlugin *plugin = findPlugin(pluginId);
    if (!plugin || plugin->status() != LuaPlugin::Status::Enabled)
        return false;

    for (const LuaCommandDef &cmd : plugin->commands()) {
        if (cmd.id == commandId && cmd.executeRef != -1) {
            UsageHistory::instance().recordLaunch(cmd.id);
            m_currentExecutingPlugin = plugin;

            lua_State *L = m_engine.state();
            lua_pushcfunction(L, LuaEngine::errorHandler);
            int errIdx = lua_gettop(L);

            lua_rawgeti(L, LUA_REGISTRYINDEX, cmd.executeRef);
            m_engine.enableWatchdog();
            int res = lua_pcall(L, 0, 0, errIdx);
            m_engine.disableWatchdog();

            m_currentExecutingPlugin = nullptr;

            if (res != LUA_OK) {
                QString err = QString::fromUtf8(lua_tostring(L, -1));
                lua_pop(L, 2);
                plugin->recordFailure(err);
                LuaApi::logToFile(plugin->id(), "Command execute error: " + err);
                return false;
            }
            lua_pop(L, 1);
            return true;
        }
    }
    return false;
}

bool LuaPluginManager::executeProvider(const QString &pluginId, const QString &providerId, const SearchResult &result)
{
    LuaPlugin *plugin = findPlugin(pluginId);
    if (!plugin || plugin->status() != LuaPlugin::Status::Enabled)
        return false;

    for (const LuaProviderDef &prov : plugin->providers()) {
        if (prov.id == providerId && prov.executeRef != -1) {
            UsageHistory::instance().recordLaunch(result.id());
            m_currentExecutingPlugin = plugin;

            lua_State *L = m_engine.state();
            lua_pushcfunction(L, LuaEngine::errorHandler);
            int errIdx = lua_gettop(L);

            lua_rawgeti(L, LUA_REGISTRYINDEX, prov.executeRef);

            // Push result table as argument
            QVariantMap resMap;
            resMap["id"] = result.id();
            resMap["title"] = result.title();
            resMap["subtitle"] = result.subtitle();
            resMap["data"] = result.metadataValue("data");
            LuaEngine::pushVariant(L, resMap);

            m_engine.enableWatchdog();
            int res = lua_pcall(L, 1, 0, errIdx);
            m_engine.disableWatchdog();

            m_currentExecutingPlugin = nullptr;

            if (res != LUA_OK) {
                QString err = QString::fromUtf8(lua_tostring(L, -1));
                lua_pop(L, 2);
                plugin->recordFailure(err);
                LuaApi::logToFile(plugin->id(), "Provider execute error: " + err);
                return false;
            }
            lua_pop(L, 1);
            return true;
        }
    }
    return false;
}

QVariantList LuaPluginManager::getPluginList() const
{
    QVariantList list;
    for (const LuaPlugin &plugin : m_plugins) {
        QVariantMap map;
        map["id"] = plugin.id();
        map["name"] = plugin.name();
        map["version"] = plugin.version();
        map["description"] = plugin.description();
        map["author"] = plugin.author();
        map["icon"] = plugin.icon();
        map["directory"] = plugin.directory();
        map["status"] = plugin.statusString();
        map["enabled"] = (plugin.status() == LuaPlugin::Status::Enabled);
        map["errorMessage"] = plugin.errorMessage();
        map["permissions"] = plugin.permissions().toList();
        map["settingsSchema"] = plugin.settingsSchema();
        list.append(map);
    }
    return list;
}

bool LuaPluginManager::setPluginEnabled(const QString &pluginId, bool enabled)
{
    ConfigService::instance().setPluginEnabled(pluginId, enabled);
    reloadAll();
    return true;
}

QVariant LuaPluginManager::getPluginSetting(const QString &pluginId, const QString &key, const QVariant &defaultValue) const
{
    return ConfigService::instance().getPluginSetting(pluginId, key, defaultValue);
}

void LuaPluginManager::setPluginSetting(const QString &pluginId, const QString &key, const QVariant &value)
{
    ConfigService::instance().setPluginSetting(pluginId, key, value);
}

QVariantMap LuaPluginManager::getPluginSettings(const QString &pluginId) const
{
    return ConfigService::instance().getPluginSettings(pluginId);
}

