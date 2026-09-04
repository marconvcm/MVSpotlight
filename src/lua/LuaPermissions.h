#pragma once

#include <QString>
#include <QStringList>
#include <QSet>

class LuaPermissions
{
public:
    static const QString FileRead;
    static const QString FileWrite;
    static const QString ProcessExecute;
    static const QString Network;
    static const QString ClipboardRead;
    static const QString ClipboardWrite;
    static const QString Notifications;
    static const QString AppLaunch;

    LuaPermissions();
    explicit LuaPermissions(const QStringList &declaredPermissions);

    bool hasPermission(const QString &permission) const;
    void grant(const QString &permission);
    void revoke(const QString &permission);
    QStringList toList() const;

private:
    QSet<QString> m_permissions;
};
