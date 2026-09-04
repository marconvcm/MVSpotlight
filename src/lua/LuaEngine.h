#pragma once

#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QList>
#include <functional>

struct lua_State;

class LuaEngine
{
public:
    LuaEngine();
    ~LuaEngine();

    bool init();
    void cleanup();

    lua_State* state() const { return m_L; }

    bool executeString(const QString &code, QString *errorMessage = nullptr);
    bool executeFile(const QString &filePath, QString *errorMessage = nullptr);

    // Watchdog hook to terminate infinite loops
    void enableWatchdog(int maxInstructions = 2000000);
    void disableWatchdog();
    bool isTimedOut() const { return m_timedOut; }

    // Helpers
    static void pushVariant(lua_State *L, const QVariant &val);
    static QVariant toVariant(lua_State *L, int index);
    static QVariantMap toVariantMap(lua_State *L, int index);
    static QVariantList toVariantList(lua_State *L, int index);

    static int errorHandler(lua_State *L);

private:
    lua_State *m_L{nullptr};
    bool m_timedOut{false};
    static void hookCallback(lua_State *L, void *ar);
};
