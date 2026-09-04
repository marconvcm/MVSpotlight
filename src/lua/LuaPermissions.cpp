#include "LuaPermissions.h"

const QString LuaPermissions::FileRead = "filesystem.read";
const QString LuaPermissions::FileWrite = "filesystem.write";
const QString LuaPermissions::ProcessExecute = "process.execute";
const QString LuaPermissions::Network = "network";
const QString LuaPermissions::ClipboardRead = "clipboard.read";
const QString LuaPermissions::ClipboardWrite = "clipboard.write";
const QString LuaPermissions::Notifications = "notifications";
const QString LuaPermissions::AppLaunch = "application.launch";

LuaPermissions::LuaPermissions()
{
}

LuaPermissions::LuaPermissions(const QStringList &declaredPermissions)
{
    for (const QString &p : declaredPermissions) {
        m_permissions.insert(p.trimmed());
    }
}

bool LuaPermissions::hasPermission(const QString &permission) const
{
    return m_permissions.contains(permission);
}

void LuaPermissions::grant(const QString &permission)
{
    m_permissions.insert(permission.trimmed());
}

void LuaPermissions::revoke(const QString &permission)
{
    m_permissions.remove(permission.trimmed());
}

QStringList LuaPermissions::toList() const
{
    return QStringList(m_permissions.begin(), m_permissions.end());
}
