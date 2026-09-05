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
    Q_PROPERTY(bool clearOnHide READ clearOnHide WRITE setClearOnHide NOTIFY clearOnHideChanged)
    Q_PROPERTY(bool frecencyEnabled READ frecencyEnabled WRITE setFrecencyEnabled NOTIFY frecencyEnabledChanged)
    Q_PROPERTY(qreal windowPositionRatio READ windowPositionRatio WRITE setWindowPositionRatio NOTIFY windowPositionRatioChanged)

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

    bool clearOnHide() const { return m_clearOnHide; }
    void setClearOnHide(bool clear);

    bool frecencyEnabled() const { return m_frecencyEnabled; }
    void setFrecencyEnabled(bool enabled);

    qreal windowPositionRatio() const { return m_windowPositionRatio; }
    void setWindowPositionRatio(qreal ratio);

    // Plugin Enablement & Configuration
    Q_INVOKABLE bool isPluginEnabled(const QString &pluginId) const;
    Q_INVOKABLE void setPluginEnabled(const QString &pluginId, bool enabled);

    Q_INVOKABLE QVariant getPluginSetting(const QString &pluginId, const QString &key, const QVariant &defaultValue = QVariant()) const;
    Q_INVOKABLE void setPluginSetting(const QString &pluginId, const QString &key, const QVariant &value);
    Q_INVOKABLE QVariantMap getPluginSettings(const QString &pluginId) const;

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
    void clearOnHideChanged();
    void frecencyEnabledChanged();
    void windowPositionRatioChanged();

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
    bool m_clearOnHide{true};
    bool m_frecencyEnabled{true};
    qreal m_windowPositionRatio{0.26};
};
