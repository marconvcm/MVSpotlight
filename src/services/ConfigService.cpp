#include "ConfigService.h"
#include "ProcessService.h"
#include <QDir>
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
    m_searchDebounceMs = std::clamp(m_settings.value("general/searchDebounceMs", 100).toInt(), 0, 500);
    m_clearOnHide = m_settings.value("general/clearOnHide", true).toBool();
    m_frecencyEnabled = m_settings.value("general/frecencyEnabled", true).toBool();
    m_windowPositionRatio = std::clamp(m_settings.value("general/windowPositionRatio", 0.26).toDouble(), 0.15, 0.45);

    m_aiProvider = m_settings.value("ai/provider", "gemini").toString();
    m_aiApiKey = m_settings.value("ai/apiKey", "").toString();
    m_aiModel = m_settings.value("ai/model", defaultModelForProvider(m_aiProvider)).toString();
    m_aiEndpoint = m_settings.value("ai/endpoint", "").toString();
    m_aiSystemPrompt = m_settings.value("ai/systemPrompt", "You are an intelligent desktop assistant. Give concise, direct, and helpful answers.").toString();
    m_aiTemperature = std::clamp(m_settings.value("ai/temperature", 0.7).toDouble(), 0.0, 2.0);
    m_aiAccentColor = m_settings.value("ai/accentColor", "#8A2BE2").toString();
    m_aiMaxTokens = std::clamp(m_settings.value("ai/maxTokens", 1024).toInt(), 128, 8192);
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
    m_settings.setValue("general/searchDebounceMs", m_searchDebounceMs);
    m_settings.setValue("general/clearOnHide", m_clearOnHide);
    m_settings.setValue("general/frecencyEnabled", m_frecencyEnabled);
    m_settings.setValue("general/windowPositionRatio", m_windowPositionRatio);

    m_settings.setValue("ai/provider", m_aiProvider);
    m_settings.setValue("ai/apiKey", m_aiApiKey);
    m_settings.setValue("ai/model", m_aiModel);
    m_settings.setValue("ai/endpoint", m_aiEndpoint);
    m_settings.setValue("ai/systemPrompt", m_aiSystemPrompt);
    m_settings.setValue("ai/temperature", m_aiTemperature);
    m_settings.setValue("ai/accentColor", m_aiAccentColor);
    m_settings.setValue("ai/maxTokens", m_aiMaxTokens);

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

void ConfigService::setSearchDebounceMs(int ms)
{
    ms = std::clamp(ms, 0, 500);
    if (m_searchDebounceMs == ms) return;
    m_searchDebounceMs = ms;
    save();
    emit searchDebounceMsChanged();
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

QString ConfigService::chooseDirectory(const QString &title, const QString &initialPath)
{
    QString initial = initialPath.trimmed();
    if (initial.startsWith("~/")) {
        initial = QDir::homePath() + initial.mid(1);
    } else if (initial == "~" || initial.isEmpty()) {
        initial = QDir::homePath();
    }

    QStringList args = {"--file-selection", "--directory"};
    if (!title.isEmpty()) {
        args << ("--title=" + title);
    }
    if (!initial.isEmpty() && QDir(initial).exists()) {
        if (!initial.endsWith('/')) {
            initial += '/';
        }
        args << ("--filename=" + initial);
    }

    ProcessResult res = ProcessService::instance().run("zenity", args, 60000);
    if (res.success && !res.stdoutOutput.trimmed().isEmpty()) {
        return res.stdoutOutput.trimmed();
    }
    return QString();
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

void ConfigService::setAiProvider(const QString &provider)
{
    if (m_aiProvider == provider) return;
    QString oldProvider = m_aiProvider;
    m_aiProvider = provider;
    bool isOldDefault = m_aiModel.isEmpty() ||
                        (oldProvider == "gemini" && m_aiModel.contains("gemini")) ||
                        (oldProvider == "openai" && (m_aiModel.contains("gpt") || m_aiModel.contains("o1") || m_aiModel.contains("o3"))) ||
                        (oldProvider == "claude" && m_aiModel.contains("claude")) ||
                        (oldProvider == "ollama" && (m_aiModel.contains("llama") || m_aiModel.contains("mistral")));
    if (isOldDefault) {
        m_aiModel = defaultModelForProvider(provider);
        emit aiModelChanged();
    }
    save();
    emit aiProviderChanged();
}

void ConfigService::setAiApiKey(const QString &key)
{
    if (m_aiApiKey == key) return;
    m_aiApiKey = key.trimmed();
    save();
    emit aiApiKeyChanged();
}

void ConfigService::setAiModel(const QString &model)
{
    if (m_aiModel == model) return;
    m_aiModel = model.trimmed();
    save();
    emit aiModelChanged();
}

void ConfigService::setAiEndpoint(const QString &endpoint)
{
    if (m_aiEndpoint == endpoint) return;
    m_aiEndpoint = endpoint.trimmed();
    save();
    emit aiEndpointChanged();
}

void ConfigService::setAiSystemPrompt(const QString &prompt)
{
    if (m_aiSystemPrompt == prompt) return;
    m_aiSystemPrompt = prompt;
    save();
    emit aiSystemPromptChanged();
}

void ConfigService::setAiTemperature(qreal temp)
{
    qreal clamped = std::clamp(temp, 0.0, 2.0);
    if (qFuzzyCompare(m_aiTemperature, clamped)) return;
    m_aiTemperature = clamped;
    save();
    emit aiTemperatureChanged();
}

void ConfigService::setAiAccentColor(const QString &color)
{
    if (m_aiAccentColor == color) return;
    m_aiAccentColor = color;
    save();
    emit aiAccentColorChanged();
}

void ConfigService::setAiMaxTokens(int tokens)
{
    int clamped = std::clamp(tokens, 128, 8192);
    if (m_aiMaxTokens == clamped) return;
    m_aiMaxTokens = clamped;
    save();
    emit aiMaxTokensChanged();
}

QString ConfigService::aiProviderName() const
{
    if (m_aiProvider == "gemini") return "Google Gemini";
    if (m_aiProvider == "openai") return "OpenAI";
    if (m_aiProvider == "claude") return "Anthropic Claude";
    if (m_aiProvider == "ollama") return "Ollama Local";
    if (m_aiProvider == "custom") return "Custom AI";
    return "AI Assistant";
}

QString ConfigService::defaultModelForProvider(const QString &provider)
{
    if (provider == "gemini") return "gemini-2.0-flash";
    if (provider == "openai") return "gpt-4o-mini";
    if (provider == "claude") return "claude-3-5-sonnet-20241022";
    if (provider == "ollama") return "llama3.2";
    return "default-model";
}

void ConfigService::resetAllToDefaults()
{
    resetAppearanceToDefaults();
    m_maxResults = 8;
    m_searchDebounceMs = 100;
    m_clearOnHide = true;
    m_frecencyEnabled = true;
    m_windowPositionRatio = 0.26;

    m_aiProvider = "gemini";
    m_aiApiKey = "";
    m_aiModel = "gemini-2.0-flash";
    m_aiEndpoint = "";
    m_aiSystemPrompt = "You are an intelligent desktop assistant. Give concise, direct, and helpful answers.";
    m_aiTemperature = 0.7;
    m_aiAccentColor = "#8A2BE2";
    m_aiMaxTokens = 1024;
    save();

    emit maxResultsChanged();
    emit searchDebounceMsChanged();
    emit clearOnHideChanged();
    emit frecencyEnabledChanged();
    emit windowPositionRatioChanged();

    emit aiProviderChanged();
    emit aiApiKeyChanged();
    emit aiModelChanged();
    emit aiEndpointChanged();
    emit aiSystemPromptChanged();
    emit aiTemperatureChanged();
    emit aiAccentColorChanged();
    emit aiMaxTokensChanged();
}
