#include "LuaEngine.h"

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#include <QDebug>
#include <QFile>

LuaEngine::LuaEngine()
{
}

LuaEngine::~LuaEngine()
{
    cleanup();
}

void LuaEngine::hookCallback(lua_State *L, void *ar)
{
    Q_UNUSED(ar);
    lua_pushstring(L, "Lua execution timed out (instruction limit exceeded)");
    lua_error(L);
}

int LuaEngine::errorHandler(lua_State *L)
{
    const char *msg = lua_tostring(L, 1);
    luaL_traceback(L, L, msg, 1);
    return 1;
}

bool LuaEngine::init()
{
    cleanup();

    m_L = luaL_newstate();
    if (!m_L) {
        qWarning() << "Failed to create Lua state";
        return false;
    }

    // Open standard libraries
    luaL_openlibs(m_L);

    // Sandboxing: disable os.execute, os.exit, os.remove, os.rename, etc.
    lua_getglobal(m_L, "os");
    if (lua_istable(m_L, -1)) {
        lua_pushnil(m_L);
        lua_setfield(m_L, -2, "execute");
        lua_pushnil(m_L);
        lua_setfield(m_L, -2, "exit");
        lua_pushnil(m_L);
        lua_setfield(m_L, -2, "remove");
        lua_pushnil(m_L);
        lua_setfield(m_L, -2, "rename");
        lua_pushnil(m_L);
        lua_setfield(m_L, -2, "tmpname");
    }
    lua_pop(m_L, 1);

    return true;
}

void LuaEngine::cleanup()
{
    if (m_L) {
        lua_close(m_L);
        m_L = nullptr;
    }
}

void LuaEngine::enableWatchdog(int maxInstructions)
{
    if (!m_L) return;
    m_timedOut = false;
    lua_sethook(m_L, (lua_Hook)hookCallback, LUA_MASKCOUNT, maxInstructions);
}

void LuaEngine::disableWatchdog()
{
    if (!m_L) return;
    lua_sethook(m_L, nullptr, 0, 0);
}

bool LuaEngine::executeString(const QString &code, QString *errorMessage)
{
    if (!m_L) return false;

    // Push custom error handler
    lua_pushcfunction(m_L, errorHandler);
    int errIdx = lua_gettop(m_L);

    if (luaL_loadstring(m_L, code.toUtf8().constData()) != LUA_OK) {
        QString err = QString::fromUtf8(lua_tostring(m_L, -1));
        if (errorMessage) *errorMessage = err;
        lua_pop(m_L, 2); // pop error and handler
        return false;
    }

    enableWatchdog();
    int res = lua_pcall(m_L, 0, LUA_MULTRET, errIdx);
    disableWatchdog();

    if (res != LUA_OK) {
        QString err = QString::fromUtf8(lua_tostring(m_L, -1));
        if (errorMessage) *errorMessage = err;
        lua_pop(m_L, 2);
        return false;
    }

    lua_pop(m_L, 1); // pop error handler
    return true;
}

bool LuaEngine::executeFile(const QString &filePath, QString *errorMessage)
{
    if (!m_L) return false;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) *errorMessage = "Cannot open file: " + filePath;
        return false;
    }

    QByteArray bytes = file.readAll();
    file.close();

    lua_pushcfunction(m_L, errorHandler);
    int errIdx = lua_gettop(m_L);

    if (luaL_loadbuffer(m_L, bytes.constData(), bytes.size(), filePath.toUtf8().constData()) != LUA_OK) {
        QString err = QString::fromUtf8(lua_tostring(m_L, -1));
        if (errorMessage) *errorMessage = err;
        lua_pop(m_L, 2);
        return false;
    }

    enableWatchdog();
    int res = lua_pcall(m_L, 0, LUA_MULTRET, errIdx);
    disableWatchdog();

    if (res != LUA_OK) {
        QString err = QString::fromUtf8(lua_tostring(m_L, -1));
        if (errorMessage) *errorMessage = err;
        lua_pop(m_L, 2);
        return false;
    }

    lua_pop(m_L, 1);
    return true;
}

void LuaEngine::pushVariant(lua_State *L, const QVariant &val)
{
    switch (val.typeId()) {
    case QMetaType::Bool:
        lua_pushboolean(L, val.toBool());
        break;
    case QMetaType::Int:
    case QMetaType::LongLong:
        lua_pushinteger(L, val.toLongLong());
        break;
    case QMetaType::Double:
    case QMetaType::Float:
        lua_pushnumber(L, val.toDouble());
        break;
    case QMetaType::QString:
        lua_pushstring(L, val.toString().toUtf8().constData());
        break;
    case QMetaType::QVariantList:
    case QMetaType::QStringList: {
        QVariantList list = val.toList();
        lua_createtable(L, list.size(), 0);
        for (int i = 0; i < list.size(); ++i) {
            pushVariant(L, list.at(i));
            lua_rawseti(L, -2, i + 1);
        }
        break;
    }
    case QMetaType::QVariantMap: {
        QVariantMap map = val.toMap();
        lua_createtable(L, 0, map.size());
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            pushVariant(L, it.value());
            lua_setfield(L, -2, it.key().toUtf8().constData());
        }
        break;
    }
    default:
        lua_pushnil(L);
        break;
    }
}

QVariant LuaEngine::toVariant(lua_State *L, int index)
{
    int type = lua_type(L, index);
    switch (type) {
    case LUA_TNIL:
        return QVariant();
    case LUA_TBOOLEAN:
        return QVariant(static_cast<bool>(lua_toboolean(L, index)));
    case LUA_TNUMBER:
        if (lua_isinteger(L, index)) {
            return QVariant(static_cast<qlonglong>(lua_tointeger(L, index)));
        }
        return QVariant(lua_tonumber(L, index));
    case LUA_TSTRING:
        return QVariant(QString::fromUtf8(lua_tostring(L, index)));
    case LUA_TTABLE: {
        // Check if array or map
        lua_len(L, index);
        lua_Integer len = lua_tointeger(L, -1);
        lua_pop(L, 1);
        if (len > 0) {
            return toVariantList(L, index);
        }
        return toVariantMap(L, index);
    }
    default:
        return QVariant();
    }
}

QVariantMap LuaEngine::toVariantMap(lua_State *L, int index)
{
    QVariantMap map;
    if (!lua_istable(L, index)) return map;

    int absIdx = lua_absindex(L, index);
    lua_pushnil(L);
    while (lua_next(L, absIdx) != 0) {
        QString key;
        if (lua_type(L, -2) == LUA_TSTRING) {
            key = QString::fromUtf8(lua_tostring(L, -2));
        } else if (lua_type(L, -2) == LUA_TNUMBER) {
            key = QString::number(lua_tointeger(L, -2));
        }
        if (!key.isEmpty()) {
            map.insert(key, toVariant(L, -1));
        }
        lua_pop(L, 1);
    }
    return map;
}

QVariantList LuaEngine::toVariantList(lua_State *L, int index)
{
    QVariantList list;
    if (!lua_istable(L, index)) return list;

    int absIdx = lua_absindex(L, index);
    lua_len(L, absIdx);
    lua_Integer len = lua_tointeger(L, -1);
    lua_pop(L, 1);

    for (lua_Integer i = 1; i <= len; ++i) {
        lua_rawgeti(L, absIdx, i);
        list.append(toVariant(L, -1));
        lua_pop(L, 1);
    }
    return list;
}
