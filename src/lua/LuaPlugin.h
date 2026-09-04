#pragma once

#include <QString>
#include <QStringList>
#include <QList>
#include <QVariantMap>
#include "LuaPermissions.h"

struct LuaCommandDef {
    QString id;
    QString title;
    QString subtitle;
    QString icon;
    QStringList keywords;
    int executeRef{-1};
};

struct LuaProviderDef {
    QString id;
    int searchRef{-1};
    int executeRef{-1};
};

class LuaPlugin
{
public:
    enum class Status {
        Enabled,
        Disabled,
        Error
    };

    LuaPlugin();

    QString id() const { return m_id; }
    void setId(const QString &id) { m_id = id; }

    QString name() const { return m_name; }
    void setName(const QString &name) { m_name = name; }

    QString version() const { return m_version; }
    void setVersion(const QString &version) { m_version = version; }

    QString description() const { return m_description; }
    void setDescription(const QString &desc) { m_description = desc; }

    QString author() const { return m_author; }
    void setAuthor(const QString &author) { m_author = author; }

    QString entry() const { return m_entry; }
    void setEntry(const QString &entry) { m_entry = entry; }

    QString icon() const { return m_icon; }
    void setIcon(const QString &icon) { m_icon = icon; }

    int minimumApiVersion() const { return m_minimumApiVersion; }
    void setMinimumApiVersion(int v) { m_minimumApiVersion = v; }

    QString directory() const { return m_directory; }
    void setDirectory(const QString &dir) { m_directory = dir; }

    LuaPermissions permissions() const { return m_permissions; }
    void setPermissions(const LuaPermissions &perms) { m_permissions = perms; }

    Status status() const { return m_status; }
    void setStatus(Status s) { m_status = s; }
    QString statusString() const;

    QString errorMessage() const { return m_errorMessage; }
    void setErrorMessage(const QString &msg) { m_errorMessage = msg; }

    int failureCount() const { return m_failureCount; }
    void recordFailure(const QString &reason);
    void resetFailures() { m_failureCount = 0; }

    const QList<LuaCommandDef>& commands() const { return m_commands; }
    void addCommand(const LuaCommandDef &cmd) { m_commands.append(cmd); }

    const QList<LuaProviderDef>& providers() const { return m_providers; }
    void addProvider(const LuaProviderDef &prov) { m_providers.append(prov); }

    void clearRegistrations();

private:
    QString m_id;
    QString m_name;
    QString m_version;
    QString m_description;
    QString m_author;
    QString m_entry{"plugin.lua"};
    QString m_icon;
    int m_minimumApiVersion{1};
    QString m_directory;
    LuaPermissions m_permissions;

    Status m_status{Status::Disabled};
    QString m_errorMessage;
    int m_failureCount{0};

    QList<LuaCommandDef> m_commands;
    QList<LuaProviderDef> m_providers;
};
