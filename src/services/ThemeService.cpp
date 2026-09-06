#include "ThemeService.h"
#include "ConfigService.h"
#include <QGuiApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusInterface>
#include <QProcess>

ThemeService& ThemeService::instance()
{
    static ThemeService s_instance;
    return s_instance;
}

ThemeService::ThemeService(QObject *parent)
    : QObject(parent)
{
    checkGnomeTheme();
    setupGnomeListener();

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    // Listen to Qt Style hints
    if (QGuiApplication::styleHints()) {
        connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
                this, [this](Qt::ColorScheme scheme) {
            m_gnomeDark = (scheme == Qt::ColorScheme::Dark);
            if (ConfigService::instance().themeMode() == "auto") {
                updateEffectiveTheme();
            }
        });
    }
#endif

    // Connect to ConfigService appearance updates
    ConfigService &cfg = ConfigService::instance();
    connect(&cfg, &ConfigService::themeModeChanged, this, &ThemeService::updateEffectiveTheme);
    connect(&cfg, &ConfigService::accentColorChanged, this, &ThemeService::themeChanged);
    connect(&cfg, &ConfigService::surfaceOpacityChanged, this, &ThemeService::themeChanged);
    connect(&cfg, &ConfigService::cornerRadiusChanged, this, &ThemeService::themeChanged);

    updateEffectiveTheme();
}

void ThemeService::setupGnomeListener()
{
    // Listen for freedesktop portal setting changes (color-scheme)
    QDBusConnection::sessionBus().connect(
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.Settings",
        "SettingChanged",
        this,
        SLOT(checkGnomeTheme())
    );
}

void ThemeService::checkGnomeTheme()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    // First check Qt styleHints if available
    if (QGuiApplication::styleHints()) {
        Qt::ColorScheme scheme = QGuiApplication::styleHints()->colorScheme();
        if (scheme == Qt::ColorScheme::Dark) {
            m_gnomeDark = true;
            if (ConfigService::instance().themeMode() == "auto") {
                updateEffectiveTheme();
            }
            return;
        } else if (scheme == Qt::ColorScheme::Light) {
            m_gnomeDark = false;
            if (ConfigService::instance().themeMode() == "auto") {
                updateEffectiveTheme();
            }
            return;
        }
    }
#endif

    // Direct check via gsettings
    QProcess proc;
    proc.start("gsettings", {"get", "org.gnome.desktop.interface", "color-scheme"});
    if (proc.waitForFinished(1000)) {
        QString out = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
        m_gnomeDark = out.contains("prefer-dark");
        if (ConfigService::instance().themeMode() == "auto") {
            updateEffectiveTheme();
        }
    }
}

void ThemeService::updateEffectiveTheme()
{
    QString mode = ConfigService::instance().themeMode();
    bool newDark = m_gnomeDark;
    if (mode == "dark") {
        newDark = true;
    } else if (mode == "light") {
        newDark = false;
    } else {
        newDark = m_gnomeDark;
    }

    if (m_isDark != newDark) {
        m_isDark = newDark;
    }
    emit themeChanged();
}

bool ThemeService::isDark() const
{
    return m_isDark;
}

QString ThemeService::themeMode() const
{
    return ConfigService::instance().themeMode();
}

void ThemeService::toggleTheme()
{
    QString current = ConfigService::instance().themeMode();
    if (current == "auto") {
        ConfigService::instance().setThemeMode("dark");
    } else if (current == "dark") {
        ConfigService::instance().setThemeMode("light");
    } else {
        ConfigService::instance().setThemeMode("auto");
    }
}

QColor ThemeService::backgroundColor() const
{
    int alpha = static_cast<int>(surfaceOpacity() * 255.0);
    alpha = qBound(50, alpha, 255);
    if (m_isDark) {
        return QColor(28, 28, 32, alpha);
    } else {
        return QColor(245, 245, 248, alpha);
    }
}

QColor ThemeService::cardBackground() const
{
    if (m_isDark) {
        return QColor(42, 42, 48, 200);
    } else {
        return QColor(255, 255, 255, 215);
    }
}

QColor ThemeService::borderColor() const
{
    if (m_isDark) {
        return QColor(255, 255, 255, 32);
    } else {
        return QColor(0, 0, 0, 24);
    }
}

QColor ThemeService::textColor() const
{
    if (m_isDark) {
        return QColor(244, 244, 246);
    } else {
        return QColor(28, 28, 30);
    }
}

QColor ThemeService::secondaryTextColor() const
{
    if (m_isDark) {
        return QColor(156, 156, 166);
    } else {
        return QColor(110, 110, 118);
    }
}

QColor ThemeService::accentColor() const
{
    QString hex = ConfigService::instance().accentColor();
    QColor c(hex);
    if (c.isValid()) {
        return c;
    }
    return QColor(53, 132, 228); // Fallback GNOME Blue
}

QColor ThemeService::selectionColor() const
{
    QColor acc = accentColor();
    if (m_isDark) {
        return QColor(acc.red(), acc.green(), acc.blue(), 55);
    } else {
        return QColor(acc.red(), acc.green(), acc.blue(), 38);
    }
}

QColor ThemeService::searchBackground() const
{
    if (m_isDark) {
        return QColor(255, 255, 255, 14);
    } else {
        return QColor(0, 0, 0, 10);
    }
}

QColor ThemeService::shadowColor() const
{
    if (m_isDark) {
        return QColor(0, 0, 0, 140);
    } else {
        return QColor(0, 0, 0, 50);
    }
}

qreal ThemeService::surfaceOpacity() const
{
    return ConfigService::instance().surfaceOpacity();
}

int ThemeService::cornerRadius() const
{
    return ConfigService::instance().cornerRadius();
}
