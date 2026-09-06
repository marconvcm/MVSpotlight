#pragma once

#include <QObject>
#include <QString>
#include <QColor>
#include <QVariant>
#include <QVariantMap>
#include <QSettings>

class ConfigService : public QObject
{
    Q_OBJECT

    // Appearance Properties
    Q_PROPERTY(QString themeMode READ themeMode WRITE setThemeMode NOTIFY themeModeChanged)
    Q_PROPERTY(QString accentColor READ accentColor WRITE setAccentColor NOTIFY accentColorChanged)
    Q_PROPERTY(qreal surfaceOpacity READ surfaceOpacity WRITE setSurfaceOpacity NOTIFY surfaceOpacityChanged)
    Q_PROPERTY(int cornerRadius READ cornerRadius WRITE setCornerRadius NOTIFY cornerRadiusChanged)
    Q_PROPERTY(int cardWidth READ cardWidth WRITE setCardWidth NOTIFY cardWidthChanged)
    Q_PROPERTY(qreal fontSizeScale READ fontSizeScale WRITE setFontSizeScale NOTIFY fontSizeScaleChanged)

    // General Properties
    Q_PROPERTY(int maxResults READ maxResults WRITE setMaxResults NOTIFY maxResultsChanged)
    Q_PROPERTY(int searchDebounceMs READ searchDebounceMs WRITE setSearchDebounceMs NOTIFY searchDebounceMsChanged)
    Q_PROPERTY(bool clearOnHide READ clearOnHide WRITE setClearOnHide NOTIFY clearOnHideChanged)
    Q_PROPERTY(bool frecencyEnabled READ frecencyEnabled WRITE setFrecencyEnabled NOTIFY frecencyEnabledChanged)
    Q_PROPERTY(qreal windowPositionRatio READ windowPositionRatio WRITE setWindowPositionRatio NOTIFY windowPositionRatioChanged)

    // AI Properties
    Q_PROPERTY(QString aiProvider READ aiProvider WRITE setAiProvider NOTIFY aiProviderChanged)
    Q_PROPERTY(QString aiApiKey READ aiApiKey WRITE setAiApiKey NOTIFY aiApiKeyChanged)
    Q_PROPERTY(QString aiModel READ aiModel WRITE setAiModel NOTIFY aiModelChanged)
    Q_PROPERTY(QString aiEndpoint READ aiEndpoint WRITE setAiEndpoint NOTIFY aiEndpointChanged)
    Q_PROPERTY(QString aiSystemPrompt READ aiSystemPrompt WRITE setAiSystemPrompt NOTIFY aiSystemPromptChanged)
    Q_PROPERTY(qreal aiTemperature READ aiTemperature WRITE setAiTemperature NOTIFY aiTemperatureChanged)
    Q_PROPERTY(QString aiAccentColor READ aiAccentColor WRITE setAiAccentColor NOTIFY aiAccentColorChanged)
    Q_PROPERTY(int aiMaxTokens READ aiMaxTokens WRITE setAiMaxTokens NOTIFY aiMaxTokensChanged)
    Q_PROPERTY(QString aiProviderName READ aiProviderName NOTIFY aiProviderChanged)

public:
    static ConfigService& instance();

    // Appearance Getters / Setters
    QString themeMode() const { return m_themeMode; }
    void setThemeMode(const QString &mode);

    QString accentColor() const { return m_accentColor; }
    void setAccentColor(const QString &color);

    qreal surfaceOpacity() const { return m_surfaceOpacity; }
    void setSurfaceOpacity(qreal opacity);

    int cornerRadius() const { return m_cornerRadius; }
    void setCornerRadius(int radius);

    int cardWidth() const { return m_cardWidth; }
    void setCardWidth(int width);

    qreal fontSizeScale() const { return m_fontSizeScale; }
    void setFontSizeScale(qreal scale);

    // General Getters / Setters
    int maxResults() const { return m_maxResults; }
    void setMaxResults(int count);

    int searchDebounceMs() const { return m_searchDebounceMs; }
    void setSearchDebounceMs(int ms);

    bool clearOnHide() const { return m_clearOnHide; }
    void setClearOnHide(bool clear);

    bool frecencyEnabled() const { return m_frecencyEnabled; }
    void setFrecencyEnabled(bool enabled);

    qreal windowPositionRatio() const { return m_windowPositionRatio; }
    void setWindowPositionRatio(qreal ratio);

    // AI Getters / Setters
    QString aiProvider() const { return m_aiProvider; }
    void setAiProvider(const QString &provider);

    QString aiApiKey() const { return m_aiApiKey; }
    void setAiApiKey(const QString &key);

    QString aiModel() const { return m_aiModel; }
    void setAiModel(const QString &model);

    QString aiEndpoint() const { return m_aiEndpoint; }
    void setAiEndpoint(const QString &endpoint);

    QString aiSystemPrompt() const { return m_aiSystemPrompt; }
    void setAiSystemPrompt(const QString &prompt);

    qreal aiTemperature() const { return m_aiTemperature; }
    void setAiTemperature(qreal temp);

    QString aiAccentColor() const { return m_aiAccentColor; }
    void setAiAccentColor(const QString &color);

    int aiMaxTokens() const { return m_aiMaxTokens; }
    void setAiMaxTokens(int tokens);

    Q_INVOKABLE QString aiProviderName() const;
    Q_INVOKABLE static QString defaultModelForProvider(const QString &provider);

    // Plugin Enablement & Configuration
    Q_INVOKABLE bool isPluginEnabled(const QString &pluginId) const;
    Q_INVOKABLE void setPluginEnabled(const QString &pluginId, bool enabled);

    Q_INVOKABLE QVariant getPluginSetting(const QString &pluginId, const QString &key, const QVariant &defaultValue = QVariant()) const;
    Q_INVOKABLE void setPluginSetting(const QString &pluginId, const QString &key, const QVariant &value);
    Q_INVOKABLE QVariantMap getPluginSettings(const QString &pluginId) const;

    // Native folder picker
    Q_INVOKABLE QString chooseDirectory(const QString &title = QString(), const QString &initialPath = QString());

    // Reset methods
    Q_INVOKABLE void resetAppearanceToDefaults();
    Q_INVOKABLE void resetAllToDefaults();

signals:
    void themeModeChanged();
    void accentColorChanged();
    void surfaceOpacityChanged();
    void cornerRadiusChanged();
    void cardWidthChanged();
    void fontSizeScaleChanged();

    void maxResultsChanged();
    void searchDebounceMsChanged();
    void clearOnHideChanged();
    void frecencyEnabledChanged();
    void windowPositionRatioChanged();

    void aiProviderChanged();
    void aiApiKeyChanged();
    void aiModelChanged();
    void aiEndpointChanged();
    void aiSystemPromptChanged();
    void aiTemperatureChanged();
    void aiAccentColorChanged();
    void aiMaxTokensChanged();

    void pluginStateChanged(const QString &pluginId, bool enabled);
    void pluginConfigChanged(const QString &pluginId, const QString &key);
    void settingsChanged();

private:
    explicit ConfigService(QObject *parent = nullptr);
    void load();
    void save();

    QSettings m_settings;

    // Appearance defaults
    QString m_themeMode{"auto"}; // "auto", "dark", "light"
    QString m_accentColor{"#3584e4"}; // GNOME 50 Blue
    qreal m_surfaceOpacity{0.88};
    int m_cornerRadius{22};
    int m_cardWidth{680};
    qreal m_fontSizeScale{1.0};

    // General defaults
    int m_maxResults{8};
    int m_searchDebounceMs{100};
    bool m_clearOnHide{true};
    bool m_frecencyEnabled{true};
    qreal m_windowPositionRatio{0.26};

    // AI defaults
    QString m_aiProvider{"gemini"};
    QString m_aiApiKey{""};
    QString m_aiModel{"gemini-2.0-flash"};
    QString m_aiEndpoint{""};
    QString m_aiSystemPrompt{"You are an intelligent desktop assistant. Give concise, direct, and helpful answers."};
    qreal m_aiTemperature{0.7};
    QString m_aiAccentColor{"#8A2BE2"};
    int m_aiMaxTokens{1024};
};
