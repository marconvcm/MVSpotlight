#pragma once

#include <QDBusAbstractAdaptor>
#include <QString>
#include <QStringList>

class SearchController;

class MVSpotlightAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mvspotlight.Launcher")

public:
    explicit MVSpotlightAdaptor(SearchController *controller);

public slots:
    void Show();
    void Hide();
    void Toggle();
    void Search(const QString &query);
    void ReloadPlugins();
    QStringList ListPlugins();

signals:
    void Shown();
    void Hidden();

private:
    SearchController *m_controller;
};

class SpotlightAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.example.Spotlight")

public:
    explicit SpotlightAdaptor(SearchController *controller);

public slots:
    void Show();
    void Hide();
    void Toggle();
    void Search(const QString &query);
    void ReloadPlugins();
    QStringList ListPlugins();

signals:
    void Shown();
    void Hidden();

private:
    SearchController *m_controller;
};
