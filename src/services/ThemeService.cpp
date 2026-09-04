#include "ThemeService.h"
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

    if (QGuiApplication::styleHints()) {
        connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
                this, [this](Qt::ColorScheme scheme) {
            if (!m_manualOverride) {
                bool dark = (scheme == Qt::ColorScheme::Dark);
                if (m_isDark != dark) {
                    m_isDark = dark;
                    emit themeChanged();
                }
            }
        });
    }
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
    if (m_manualOverride)
        return;

    // First check Qt styleHints if available
    if (QGuiApplication::styleHints()) {
        Qt::ColorScheme scheme = QGuiApplication::styleHints()->colorScheme();
        if (scheme == Qt::ColorScheme::Dark) {
            if (!m_isDark) {
                m_isDark = true;
                emit themeChanged();
            }
            return;
        } else if (scheme == Qt::ColorScheme::Light) {
            if (m_isDark) {
                m_isDark = false;
                emit themeChanged();
            }
            return;
        }
    }

    // Direct check via gsettings
    QProcess proc;
    proc.start("gsettings", {"get", "org.gnome.desktop.interface", "color-scheme"});
    if (proc.waitForFinished(1000)) {
        QString out = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
        bool dark = out.contains("prefer-dark");
        if (m_isDark != dark) {
            m_isDark = dark;
            emit themeChanged();
        }
    }
}

void ThemeService::toggleTheme()
{
    m_manualOverride = true;
    m_isDark = !m_isDark;
    emit themeChanged();
}

QColor ThemeService::backgroundColor() const
{
    if (m_isDark) {
        return QColor(28, 28, 32, 226); // ~0.88 opacity charcoal
    } else {
        return QColor(245, 245, 248, 226); // ~0.88 opacity off-white
    }
}

QColor ThemeService::cardBackground() const
{
    if (m_isDark) {
        return QColor(42, 42, 48, 200);
    } else {
        return QColor(255, 255, 255, 210);
    }
}

QColor ThemeService::borderColor() const
{
    if (m_isDark) {
        return QColor(255, 255, 255, 28); // subtle white border ~11%
    } else {
        return QColor(0, 0, 0, 22); // subtle dark border ~8%
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
    return QColor(53, 132, 228); // GNOME blue
}

QColor ThemeService::selectionColor() const
{
    if (m_isDark) {
        return QColor(255, 255, 255, 30);
    } else {
        return QColor(0, 0, 0, 18);
    }
}

QColor ThemeService::searchBackground() const
{
    if (m_isDark) {
        return QColor(255, 255, 255, 12);
    } else {
        return QColor(0, 0, 0, 8);
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
