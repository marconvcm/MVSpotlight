#include "LuaApi.h"
#include "LuaPluginManager.h"
#include "../services/NotificationService.h"
#include "../services/ClipboardService.h"
#include "../services/ProcessService.h"
#include <QDesktopServices>
#include <QUrl>
#include <QSettings>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QStandardPaths>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QProcess>
#include <QDebug>

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

static QStringList variantListToStringList(const QVariantList &list)
{
    QStringList res;
    for (const auto &v : list) {
        res.append(v.toString());
    }
    return res;
}

LuaApi* LuaApi::s_currentApi = nullptr;

LuaApi::LuaApi(LuaPluginManager *manager, QObject *parent)
    : QObject(parent)
    , m_manager(manager)
{
    s_currentApi = this;
}

void LuaApi::logToFile(const QString &pluginId, const QString &message)
{
    QString stateDir = QStandardPaths::writableLocation(QStandardPaths::GenericStateLocation);
    if (stateDir.isEmpty()) stateDir = QDir::homePath() + "/.local/state";
    QDir().mkpath(stateDir + "/mvspotlight");
    QString logPath = stateDir + "/mvspotlight/launcher.log";

    QFile file(logPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz")
            << " [" << (pluginId.isEmpty() ? "SYSTEM" : pluginId) << "] "
            << message << "\n";
    }
}

void LuaApi::registerApi(lua_State *L)
{
    lua_newtable(L); // launcher table

    lua_pushcfunction(L, lua_register_command);
    lua_setfield(L, -2, "register_command");

    lua_pushcfunction(L, lua_register_provider);
    lua_setfield(L, -2, "register_provider");

    lua_pushcfunction(L, lua_notify);
    lua_setfield(L, -2, "notify");

    lua_pushcfunction(L, lua_copy_to_clipboard);
    lua_setfield(L, -2, "copy_to_clipboard");

    lua_pushcfunction(L, lua_get_clipboard);
    lua_setfield(L, -2, "get_clipboard");

    lua_pushcfunction(L, lua_open_url);
    lua_setfield(L, -2, "open_url");

    lua_pushcfunction(L, lua_open_file);
    lua_setfield(L, -2, "open_file");

    lua_pushcfunction(L, lua_launch_application);
    lua_setfield(L, -2, "launch_application");

    lua_pushcfunction(L, lua_get_config);
    lua_setfield(L, -2, "get_config");

    lua_pushcfunction(L, lua_set_config);
    lua_setfield(L, -2, "set_config");

    lua_pushcfunction(L, lua_log);
    lua_setfield(L, -2, "log");

    // launcher.process table
    lua_newtable(L);
    lua_pushcfunction(L, lua_process_run);
    lua_setfield(L, -2, "run");
    lua_pushcfunction(L, lua_process_run_async);
    lua_setfield(L, -2, "run_async");
    lua_setfield(L, -2, "process");

    // launcher.http table
    lua_newtable(L);
    lua_pushcfunction(L, lua_http_get);
    lua_setfield(L, -2, "get");
    lua_setfield(L, -2, "http");

    lua_setglobal(L, "launcher");
}

int LuaApi::lua_register_command(lua_State *L)
{
    if (!s_currentApi || !s_currentApi->m_manager) return 0;
    LuaPlugin *plugin = s_currentApi->m_manager->currentLoadingPlugin();
    if (!plugin) {
        luaL_error(L, "register_command can only be called during plugin initialization");
        return 0;
    }

    if (!lua_istable(L, 1)) {
        luaL_error(L, "Expected table argument for register_command");
        return 0;
    }

    LuaCommandDef cmd;

    lua_getfield(L, 1, "id");
    if (lua_isstring(L, -1)) cmd.id = QString::fromUtf8(lua_tostring(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, 1, "title");
    if (lua_isstring(L, -1)) cmd.title = QString::fromUtf8(lua_tostring(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, 1, "subtitle");
    if (lua_isstring(L, -1)) cmd.subtitle = QString::fromUtf8(lua_tostring(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, 1, "icon");
    if (lua_isstring(L, -1)) cmd.icon = QString::fromUtf8(lua_tostring(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, 1, "keywords");
    if (lua_istable(L, -1)) {
        cmd.keywords = variantListToStringList(LuaEngine::toVariantList(L, -1));
    }
    lua_pop(L, 1);

    lua_getfield(L, 1, "execute");
    if (lua_isfunction(L, -1)) {
        cmd.executeRef = luaL_ref(L, LUA_REGISTRYINDEX);
    } else {
        lua_pop(L, 1);
    }

    if (cmd.id.isEmpty()) {
        cmd.id = plugin->id() + "." + cmd.title.toLower().replace(' ', '-');
    }
    if (cmd.icon.isEmpty()) {
        cmd.icon = plugin->icon().isEmpty() ? "application-x-executable" : plugin->icon();
    }

    plugin->addCommand(cmd);
    return 0;
}

int LuaApi::lua_register_provider(lua_State *L)
{
    if (!s_currentApi || !s_currentApi->m_manager) return 0;
    LuaPlugin *plugin = s_currentApi->m_manager->currentLoadingPlugin();
    if (!plugin) {
        luaL_error(L, "register_provider can only be called during plugin initialization");
        return 0;
    }

    if (!lua_istable(L, 1)) {
        luaL_error(L, "Expected table argument for register_provider");
        return 0;
    }

    LuaProviderDef prov;

    lua_getfield(L, 1, "id");
    if (lua_isstring(L, -1)) prov.id = QString::fromUtf8(lua_tostring(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, 1, "search");
    if (lua_isfunction(L, -1)) {
        prov.searchRef = luaL_ref(L, LUA_REGISTRYINDEX);
    } else {
        lua_pop(L, 1);
    }

    lua_getfield(L, 1, "execute");
    if (lua_isfunction(L, -1)) {
        prov.executeRef = luaL_ref(L, LUA_REGISTRYINDEX);
    } else {
        lua_pop(L, 1);
    }

    if (prov.id.isEmpty()) {
        prov.id = plugin->id();
    }

    plugin->addProvider(prov);
    return 0;
}

int LuaApi::lua_notify(lua_State *L)
{
    if (!s_currentApi || !s_currentApi->m_manager) return 0;
    LuaPlugin *plugin = s_currentApi->m_manager->currentExecutingPlugin();
    if (plugin && !plugin->permissions().hasPermission(LuaPermissions::Notifications)) {
        logToFile(plugin->id(), "Permission denied: notifications");
        return luaL_error(L, "Permission denied: 'notifications' permission required");
    }

    const char *title = luaL_optstring(L, 1, "Spotlight");
    const char *body = luaL_optstring(L, 2, "");
    const char *icon = luaL_optstring(L, 3, "dialog-information");

    NotificationService::instance().notify(QString::fromUtf8(title), QString::fromUtf8(body), QString::fromUtf8(icon));
    return 0;
}

int LuaApi::lua_copy_to_clipboard(lua_State *L)
{
    if (!s_currentApi || !s_currentApi->m_manager) return 0;
    LuaPlugin *plugin = s_currentApi->m_manager->currentExecutingPlugin();
    if (plugin && !plugin->permissions().hasPermission(LuaPermissions::ClipboardWrite)) {
        logToFile(plugin->id(), "Permission denied: clipboard.write");
        return luaL_error(L, "Permission denied: 'clipboard.write' permission required");
    }

    const char *text = luaL_checkstring(L, 1);
    ClipboardService::instance().setText(QString::fromUtf8(text));
    return 0;
}

int LuaApi::lua_get_clipboard(lua_State *L)
{
    if (!s_currentApi || !s_currentApi->m_manager) return 0;
    LuaPlugin *plugin = s_currentApi->m_manager->currentExecutingPlugin();
    if (plugin && !plugin->permissions().hasPermission(LuaPermissions::ClipboardRead)) {
        logToFile(plugin->id(), "Permission denied: clipboard.read");
        return luaL_error(L, "Permission denied: 'clipboard.read' permission required");
    }

    QString text = ClipboardService::instance().text();
    lua_pushstring(L, text.toUtf8().constData());
    return 1;
}

int LuaApi::lua_open_url(lua_State *L)
{
    if (!s_currentApi || !s_currentApi->m_manager) return 0;
    LuaPlugin *plugin = s_currentApi->m_manager->currentExecutingPlugin();
    if (plugin && !plugin->permissions().hasPermission(LuaPermissions::AppLaunch)
               && !plugin->permissions().hasPermission(LuaPermissions::Network)) {
        logToFile(plugin->id(), "Permission denied: application.launch or network");
        return luaL_error(L, "Permission denied: 'application.launch' or 'network' permission required");
    }

    const char *urlStr = luaL_checkstring(L, 1);
    bool ok = QDesktopServices::openUrl(QUrl(QString::fromUtf8(urlStr)));
    lua_pushboolean(L, ok);
    return 1;
}

int LuaApi::lua_open_file(lua_State *L)
{
    if (!s_currentApi || !s_currentApi->m_manager) return 0;
    LuaPlugin *plugin = s_currentApi->m_manager->currentExecutingPlugin();
    if (plugin && !plugin->permissions().hasPermission(LuaPermissions::FileRead)) {
        logToFile(plugin->id(), "Permission denied: filesystem.read");
        return luaL_error(L, "Permission denied: 'filesystem.read' permission required");
    }

    const char *pathStr = luaL_checkstring(L, 1);
    bool ok = QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromUtf8(pathStr)));
    lua_pushboolean(L, ok);
    return 1;
}

int LuaApi::lua_launch_application(lua_State *L)
{
    if (!s_currentApi || !s_currentApi->m_manager) return 0;
    LuaPlugin *plugin = s_currentApi->m_manager->currentExecutingPlugin();
    if (plugin && !plugin->permissions().hasPermission(LuaPermissions::AppLaunch)) {
        logToFile(plugin->id(), "Permission denied: application.launch");
        return luaL_error(L, "Permission denied: 'application.launch' permission required");
    }

    const char *execStr = luaL_checkstring(L, 1);
    QString command = QString::fromUtf8(execStr);
    QStringList parts = QProcess::splitCommand(command);
    if (parts.isEmpty()) {
        lua_pushboolean(L, false);
        return 1;
    }
    QString prog = parts.takeFirst();
    bool ok = ProcessService::instance().launchDetached(prog, parts);
    lua_pushboolean(L, ok);
    return 1;
}

int LuaApi::lua_get_config(lua_State *L)
{
    if (!s_currentApi || !s_currentApi->m_manager) return 0;
    LuaPlugin *plugin = s_currentApi->m_manager->currentExecutingPlugin();
    QString pluginId = plugin ? plugin->id() : "global";

    const char *key = luaL_checkstring(L, 1);
    QSettings settings("mvspotlight", "plugins");
    QString fullKey = "plugins/" + pluginId + "/" + QString::fromUtf8(key);

    if (settings.contains(fullKey)) {
        QVariant val = settings.value(fullKey);
        LuaEngine::pushVariant(L, val);
        return 1;
    }

    // Fallback to legacy settings
    QSettings legacySettings("spotlight-qt", "plugins");
    if (legacySettings.contains(fullKey)) {
        QVariant val = legacySettings.value(fullKey);
        LuaEngine::pushVariant(L, val);
        return 1;
    }

    // Return default value if provided
    if (lua_gettop(L) >= 2) {
        lua_pushvalue(L, 2);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}

int LuaApi::lua_set_config(lua_State *L)
{
    if (!s_currentApi || !s_currentApi->m_manager) return 0;
    LuaPlugin *plugin = s_currentApi->m_manager->currentExecutingPlugin();
    QString pluginId = plugin ? plugin->id() : "global";

    const char *key = luaL_checkstring(L, 1);
    QVariant val = LuaEngine::toVariant(L, 2);

    QSettings settings("mvspotlight", "plugins");
    QString fullKey = "plugins/" + pluginId + "/" + QString::fromUtf8(key);
    settings.setValue(fullKey, val);
    settings.sync();
    return 0;
}

int LuaApi::lua_log(lua_State *L)
{
    if (!s_currentApi || !s_currentApi->m_manager) return 0;
    LuaPlugin *plugin = s_currentApi->m_manager->currentExecutingPlugin();
    QString pluginId = plugin ? plugin->id() : "system";

    const char *msg = luaL_optstring(L, 1, "");
    logToFile(pluginId, QString::fromUtf8(msg));
    return 0;
}

int LuaApi::lua_process_run(lua_State *L)
{
    if (!s_currentApi || !s_currentApi->m_manager) return 0;
    LuaPlugin *plugin = s_currentApi->m_manager->currentExecutingPlugin();
    if (plugin && !plugin->permissions().hasPermission(LuaPermissions::ProcessExecute)) {
        logToFile(plugin->id(), "Permission denied: process.execute");
        return luaL_error(L, "Permission denied: 'process.execute' permission required");
    }

    if (!lua_istable(L, 1)) {
        return luaL_error(L, "Expected table argument for launcher.process.run");
    }

    QString command;
    QStringList args;
    int timeoutMs = 5000;
    QString workingDir;

    lua_getfield(L, 1, "command");
    if (lua_isstring(L, -1)) command = QString::fromUtf8(lua_tostring(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, 1, "arguments");
    if (lua_istable(L, -1)) {
        args = variantListToStringList(LuaEngine::toVariantList(L, -1));
    }
    lua_pop(L, 1);

    lua_getfield(L, 1, "timeout");
    if (lua_isnumber(L, -1)) timeoutMs = static_cast<int>(lua_tointeger(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, 1, "working_dir");
    if (lua_isstring(L, -1)) workingDir = QString::fromUtf8(lua_tostring(L, -1));
    lua_pop(L, 1);

    ProcessResult res = ProcessService::instance().run(command, args, timeoutMs, workingDir);

    lua_newtable(L);
    lua_pushboolean(L, res.success);
    lua_setfield(L, -2, "success");

    lua_pushinteger(L, res.exitCode);
    lua_setfield(L, -2, "exit_code");

    lua_pushstring(L, res.stdoutOutput.toUtf8().constData());
    lua_setfield(L, -2, "stdout");

    lua_pushstring(L, res.stderrOutput.toUtf8().constData());
    lua_setfield(L, -2, "stderr");

    if (!res.errorMessage.isEmpty()) {
        lua_pushstring(L, res.errorMessage.toUtf8().constData());
        lua_setfield(L, -2, "error");
    }

    return 1;
}

int LuaApi::lua_process_run_async(lua_State *L)
{
    if (!s_currentApi || !s_currentApi->m_manager) return 0;
    LuaPlugin *plugin = s_currentApi->m_manager->currentExecutingPlugin();
    if (plugin && !plugin->permissions().hasPermission(LuaPermissions::ProcessExecute)) {
        logToFile(plugin->id(), "Permission denied: process.execute");
        return luaL_error(L, "Permission denied: 'process.execute' permission required");
    }

    if (!lua_istable(L, 1)) {
        return luaL_error(L, "Expected table argument for launcher.process.run_async");
    }

    QString command;
    QStringList args;
    int timeoutMs = 10000;
    QString workingDir;
    int cbRef = -1;

    lua_getfield(L, 1, "command");
    if (lua_isstring(L, -1)) command = QString::fromUtf8(lua_tostring(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, 1, "arguments");
    if (lua_istable(L, -1)) {
        args = variantListToStringList(LuaEngine::toVariantList(L, -1));
    }
    lua_pop(L, 1);

    lua_getfield(L, 1, "timeout");
    if (lua_isnumber(L, -1)) timeoutMs = static_cast<int>(lua_tointeger(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, 1, "on_complete");
    if (lua_isfunction(L, -1)) {
        cbRef = luaL_ref(L, LUA_REGISTRYINDEX);
    } else {
        lua_pop(L, 1);
    }

    if (cbRef == -1) {
        return luaL_error(L, "Missing 'on_complete' function in run_async");
    }

    LuaPluginManager *mgr = s_currentApi->m_manager;
    QString pId = plugin ? plugin->id() : "";

    ProcessService::instance().runAsync(command, args, [mgr, cbRef, pId](const ProcessResult &res) {
        lua_State *state = mgr->engine()->state();
        if (!state) return;

        lua_rawgeti(state, LUA_REGISTRYINDEX, cbRef);
        luaL_unref(state, LUA_REGISTRYINDEX, cbRef);

        lua_newtable(state);
        lua_pushboolean(state, res.success);
        lua_setfield(state, -2, "success");
        lua_pushinteger(state, res.exitCode);
        lua_setfield(state, -2, "exit_code");
        lua_pushstring(state, res.stdoutOutput.toUtf8().constData());
        lua_setfield(state, -2, "stdout");
        lua_pushstring(state, res.stderrOutput.toUtf8().constData());
        lua_setfield(state, -2, "stderr");

        LuaPlugin *p = mgr->findPlugin(pId);
        mgr->setCurrentExecutingPlugin(p);
        mgr->engine()->enableWatchdog();
        if (lua_pcall(state, 1, 0, 0) != LUA_OK) {
            QString err = QString::fromUtf8(lua_tostring(state, -1));
            lua_pop(state, 1);
            LuaApi::logToFile(pId, "Async process callback error: " + err);
        }
        mgr->engine()->disableWatchdog();
        mgr->setCurrentExecutingPlugin(nullptr);
    }, timeoutMs, workingDir);

    return 0;
}

int LuaApi::lua_http_get(lua_State *L)
{
    if (!s_currentApi || !s_currentApi->m_manager) return 0;
    LuaPlugin *plugin = s_currentApi->m_manager->currentExecutingPlugin();
    if (plugin && !plugin->permissions().hasPermission(LuaPermissions::Network)) {
        logToFile(plugin->id(), "Permission denied: network");
        return luaL_error(L, "Permission denied: 'network' permission required");
    }

    if (!lua_istable(L, 1)) {
        return luaL_error(L, "Expected table argument for launcher.http.get");
    }

    QString urlStr;
    int cbRef = -1;

    lua_getfield(L, 1, "url");
    if (lua_isstring(L, -1)) urlStr = QString::fromUtf8(lua_tostring(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, 1, "on_complete");
    if (lua_isfunction(L, -1)) {
        cbRef = luaL_ref(L, LUA_REGISTRYINDEX);
    } else {
        lua_pop(L, 1);
    }

    if (urlStr.isEmpty() || cbRef == -1) {
        return luaL_error(L, "Invalid URL or missing 'on_complete' callback");
    }

    LuaPluginManager *mgr = s_currentApi->m_manager;
    QString pId = plugin ? plugin->id() : "";

    QNetworkRequest req((QUrl(urlStr)));
    req.setHeader(QNetworkRequest::UserAgentHeader, "MVSpotlight/1.0");

    QNetworkReply *reply = s_currentApi->m_netManager.get(req);
    QObject::connect(reply, &QNetworkReply::finished, [reply, mgr, cbRef, pId]() {
        lua_State *state = mgr->engine()->state();
        if (state) {
            lua_rawgeti(state, LUA_REGISTRYINDEX, cbRef);
            luaL_unref(state, LUA_REGISTRYINDEX, cbRef);

            int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            QString body = QString::fromUtf8(reply->readAll());

            lua_pushinteger(state, statusCode);
            lua_pushstring(state, body.toUtf8().constData());

            LuaPlugin *p = mgr->findPlugin(pId);
            mgr->setCurrentExecutingPlugin(p);
            mgr->engine()->enableWatchdog();
            if (lua_pcall(state, 2, 0, 0) != LUA_OK) {
                QString err = QString::fromUtf8(lua_tostring(state, -1));
                lua_pop(state, 1);
                LuaApi::logToFile(pId, "HTTP callback error: " + err);
            }
            mgr->engine()->disableWatchdog();
            mgr->setCurrentExecutingPlugin(nullptr);
        }
        reply->deleteLater();
    });

    return 0;
}
