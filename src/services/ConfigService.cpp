#include "ConfigService.h"
#include <algorithm>

ConfigService& ConfigService::instance()
{
    static ConfigService s_instance;
    return s_instance;
}

ConfigService::ConfigService(QObject *parent)
    : QObject(parent)
    , m_settings("mvspotlight", "settings")
{
    load();
}

void ConfigService::load()
{
    // Auto-migrate legacy settings if mvspotlight is empty
    QSettings legacySettings("spotlight-qt", "settings");
    if (!m_settings.contains("appearance/themeMode") && legacySettings.contains("appearance/themeMode")) {
        for (const QString &key : legacySettings.allKeys()) {
            m_settings.setValue(key, legacySettings.value(key));
        }
        m_settings.sync();
    }

    m_themeMode = m_settings.value("appearance/themeMode", "auto").toString();
    m_accentColor = m_settings.value("appearance/accentColor", "#3584e4").toString();
    m_surfaceOpacity = std::clamp(m_settings.value("appearance/surfaceOpacity", 0.88).toDouble(), 0.60, 1.00);
    m_cornerRadius = std::clamp(m_settings.value("appearance/cornerRadius", 22).toInt(), 10, 36);
    m_cardWidth = std::clamp(m_settings.value("appearance/cardWidth", 680).toInt(), 540, 920);
    m_fontSizeScale = std::clamp(m_settings.value("appearance/fontSizeScale", 1.00).toDouble(), 0.80, 1.35);

    m_maxResults = std::clamp(m_settings.value("general/maxResults", 8).toInt(), 3, 16);
    m_clearOnHide = m_settings.value("general/clearOnHide", true).toBool();
    m_frecencyEnabled = m_settings.value("general/frecencyEnabled", true).toBool();
    m_windowPositionRatio = std::clamp(m_settings.value("general/windowPositionRatio", 0.26).toDouble(), 0.15, 0.45);
}

void ConfigService::save()
{
    m_settings.setValue("appearance/themeMode", m_themeMode);
    m_settings.setValue("appearance/accentColor", m_accentColor);
    m_settings.setValue("appearance/surfaceOpacity", m_surfaceOpacity);
    m_settings.setValue("appearance/cornerRadius", m_cornerRadius);
    m_settings.setValue("appearance/cardWidth", m_cardWidth);
    m_settings.setValue("appearance/fontSizeScale", m_fontSizeScale);

    m_settings.setValue("general/maxResults", m_maxResults);
    m_settings.setValue("general/clearOnHide", m_clearOnHide);
    m_settings.setValue("general/frecencyEnabled", m_frecencyEnabled);
    m_settings.setValue("general/windowPositionRatio", m_windowPositionRatio);

    m_settings.sync();
    emit settingsChanged();
}

void ConfigService::setThemeMode(const QString &mode)
{
    if (m_themeMode == mode) return;
    m_themeMode = mode;
    save();
    emit themeModeChanged();
}

void ConfigService::setAccentColor(const QString &color)
{
    if (m_accentColor == color) return;
    m_accentColor = color;
    save();
    emit accentColorChanged();
}

void ConfigService::setSurfaceOpacity(qreal opacity)
{
    opacity = std::clamp(opacity, 0.60, 1.00);
    if (qFuzzyCompare(m_surfaceOpacity, opacity)) return;
    m_surfaceOpacity = opacity;
    save();
    emit surfaceOpacityChanged();
}

void ConfigService::setCornerRadius(int radius)
{
    radius = std::clamp(radius, 10, 36);
    if (m_cornerRadius == radius) return;
    m_cornerRadius = radius;
    save();
    emit cornerRadiusChanged();
}

void ConfigService::setCardWidth(int width)
{
    width = std::clamp(width, 540, 920);
    if (m_cardWidth == width) return;
    m_cardWidth = width;
    save();
    emit cardWidthChanged();
}

void ConfigService::setFontSizeScale(qreal scale)
{
    scale = std::clamp(scale, 0.80, 1.35);
    if (qFuzzyCompare(m_fontSizeScale, scale)) return;
    m_fontSizeScale = scale;
    save();
    emit fontSizeScaleChanged();
}

void ConfigService::setMaxResults(int count)
{
    count = std::clamp(count, 3, 16);
    if (m_maxResults == count) return;
    m_maxResults = count;
    save();
    emit maxResultsChanged();
}

void ConfigService::setClearOnHide(bool clear)
{
    if (m_clearOnHide == clear) return;
    m_clearOnHide = clear;
    save();
    emit clearOnHideChanged();
}

void ConfigService::setFrecencyEnabled(bool enabled)
{
    if (m_frecencyEnabled == enabled) return;
    m_frecencyEnabled = enabled;
    save();
    emit frecencyEnabledChanged();
}

void ConfigService::setWindowPositionRatio(qreal ratio)
{
    ratio = std::clamp(ratio, 0.15, 0.45);
    if (qFuzzyCompare(m_windowPositionRatio, ratio)) return;
    m_windowPositionRatio = ratio;
    save();
    emit windowPositionRatioChanged();
}

bool ConfigService::isPluginEnabled(const QString &pluginId) const
{
    QString key = "plugins/" + pluginId + "/enabled";
    return m_settings.value(key, true).toBool();
}

void ConfigService::setPluginEnabled(const QString &pluginId, bool enabled)
{
    QString key = "plugins/" + pluginId + "/enabled";
    m_settings.setValue(key, enabled);
    m_settings.sync();
    emit pluginStateChanged(pluginId, enabled);
    emit settingsChanged();
}

QVariant ConfigService::getPluginSetting(const QString &pluginId, const QString &key, const QVariant &defaultValue) const
{
    // Try structured config key
    QString fullKey = "plugins/" + pluginId + "/config/" + key;
    if (m_settings.contains(fullKey)) {
        return m_settings.value(fullKey);
    }

    // Try Lua storage path key
    QString flatKey = "plugins/" + pluginId + "/" + key;
    if (m_settings.contains(flatKey)) {
        return m_settings.value(flatKey);
    }

    // Check separate plugins QSettings file if used by LuaApi
    QSettings luaSettings("mvspotlight", "plugins");
    if (luaSettings.contains(flatKey)) {
        return luaSettings.value(flatKey);
    }

    return defaultValue;
}

void ConfigService::setPluginSetting(const QString &pluginId, const QString &key, const QVariant &value)
{
    QString fullKey = "plugins/" + pluginId + "/config/" + key;
    QString flatKey = "plugins/" + pluginId + "/" + key;

    m_settings.setValue(fullKey, value);
    m_settings.setValue(flatKey, value);
    m_settings.sync();

    // Also mirror to LuaApi storage
    QSettings luaSettings("mvspotlight", "plugins");
    luaSettings.setValue(flatKey, value);
    luaSettings.sync();

    emit pluginConfigChanged(pluginId, key);
    emit settingsChanged();
}

QVariantMap ConfigService::getPluginSettings(const QString &pluginId) const
{
    QVariantMap map;
    QString prefix = "plugins/" + pluginId + "/config/";
    for (const QString &k : m_settings.allKeys()) {
        if (k.startsWith(prefix)) {
            QString subKey = k.mid(prefix.length());
            map.insert(subKey, m_settings.value(k));
        }
    }
    return map;
}

void ConfigService::resetAppearanceToDefaults()
{
    m_themeMode = "auto";
    m_accentColor = "#3584e4";
    m_surfaceOpacity = 0.88;
    m_cornerRadius = 22;
    m_cardWidth = 680;
    m_fontSizeScale = 1.0;
    save();

    emit themeModeChanged();
    emit accentColorChanged();
    emit surfaceOpacityChanged();
    emit cornerRadiusChanged();
    emit cardWidthChanged();
    emit fontSizeScaleChanged();
}

void ConfigService::resetAllToDefaults()
{
    resetAppearanceToDefaults();
    m_maxResults = 8;
    m_clearOnHide = true;
    m_frecencyEnabled = true;
    m_windowPositionRatio = 0.26;
    save();

    emit maxResultsChanged();
    emit clearOnHideChanged();
    emit frecencyEnabledChanged();
    emit windowPositionRatioChanged();
}
