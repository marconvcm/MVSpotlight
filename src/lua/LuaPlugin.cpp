#include "LuaPlugin.h"

LuaPlugin::LuaPlugin()
{
}

QString LuaPlugin::statusString() const
{
    switch (m_status) {
    case Status::Enabled: return "Enabled";
    case Status::Disabled: return "Disabled";
    case Status::Error: return "Error";
    }
    return "Unknown";
}

void LuaPlugin::recordFailure(const QString &reason)
{
    m_failureCount++;
    m_errorMessage = reason;
    if (m_failureCount >= 3) {
        m_status = Status::Disabled;
    } else {
        m_status = Status::Error;
    }
}

void LuaPlugin::clearRegistrations()
{
    m_commands.clear();
    m_providers.clear();
}
