#pragma once

#include <QString>
#include <QObject>
#include <QNetworkAccessManager>

struct lua_State;
class LuaPluginManager;

class LuaApi : public QObject
{
    Q_OBJECT

public:
    explicit LuaApi(LuaPluginManager *manager, QObject *parent = nullptr);

    void registerApi(lua_State *L);
    static void logToFile(const QString &pluginId, const QString &message);

private:
    static int lua_register_command(lua_State *L);
    static int lua_register_provider(lua_State *L);
    static int lua_notify(lua_State *L);
    static int lua_copy_to_clipboard(lua_State *L);
    static int lua_get_clipboard(lua_State *L);
    static int lua_open_url(lua_State *L);
    static int lua_open_file(lua_State *L);
    static int lua_launch_application(lua_State *L);
    static int lua_get_config(lua_State *L);
    static int lua_set_config(lua_State *L);
    static int lua_log(lua_State *L);
    static int lua_process_run(lua_State *L);
    static int lua_process_run_async(lua_State *L);
    static int lua_http_get(lua_State *L);

    LuaPluginManager *m_manager;
    QNetworkAccessManager m_netManager;

    static LuaApi *s_currentApi;
};
