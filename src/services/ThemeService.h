#pragma once

#include <QObject>
#include <QColor>
#include <QStyleHints>

class ThemeService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isDark READ isDark NOTIFY themeChanged)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor NOTIFY themeChanged)
    Q_PROPERTY(QColor cardBackground READ cardBackground NOTIFY themeChanged)
    Q_PROPERTY(QColor borderColor READ borderColor NOTIFY themeChanged)
    Q_PROPERTY(QColor textColor READ textColor NOTIFY themeChanged)
    Q_PROPERTY(QColor secondaryTextColor READ secondaryTextColor NOTIFY themeChanged)
    Q_PROPERTY(QColor accentColor READ accentColor NOTIFY themeChanged)
    Q_PROPERTY(QColor selectionColor READ selectionColor NOTIFY themeChanged)
    Q_PROPERTY(QColor searchBackground READ searchBackground NOTIFY themeChanged)
    Q_PROPERTY(QColor shadowColor READ shadowColor NOTIFY themeChanged)
    Q_PROPERTY(qreal surfaceOpacity READ surfaceOpacity CONSTANT)
    Q_PROPERTY(int cornerRadius READ cornerRadius CONSTANT)

public:
    static ThemeService& instance();

    bool isDark() const { return m_isDark; }
    QColor backgroundColor() const;
    QColor cardBackground() const;
    QColor borderColor() const;
    QColor textColor() const;
    QColor secondaryTextColor() const;
    QColor accentColor() const;
    QColor selectionColor() const;
    QColor searchBackground() const;
    QColor shadowColor() const;
    qreal surfaceOpacity() const { return 0.88; }
    int cornerRadius() const { return 22; }

    Q_INVOKABLE void toggleTheme();

signals:
    void themeChanged();

public slots:
    void checkGnomeTheme();

private:
    explicit ThemeService(QObject *parent = nullptr);
    void setupGnomeListener();

    bool m_isDark{true};
    bool m_manualOverride{false};
};
