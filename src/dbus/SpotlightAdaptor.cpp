#include "SpotlightAdaptor.h"
#include "../core/SearchController.h"
#include "../lua/LuaPluginManager.h"

MVSpotlightAdaptor::MVSpotlightAdaptor(SearchController *controller)
    : QDBusAbstractAdaptor(controller)
    , m_controller(controller)
{
    connect(m_controller, &SearchController::windowVisibleChanged, this, [this]() {
        if (m_controller->isWindowVisible()) {
            emit Shown();
        } else {
            emit Hidden();
        }
    });
}

void MVSpotlightAdaptor::Show()
{
    m_controller->showWindow();
}

void MVSpotlightAdaptor::Hide()
{
    m_controller->hideWindow();
}

void MVSpotlightAdaptor::Toggle()
{
    m_controller->toggleWindow();
}

void MVSpotlightAdaptor::Search(const QString &query)
{
    m_controller->showWindow();
    m_controller->setQuery(query);
}

void MVSpotlightAdaptor::ReloadPlugins()
{
    m_controller->reloadPlugins();
}

QStringList MVSpotlightAdaptor::ListPlugins()
{
    QStringList result;
    if (m_controller->pluginManager()) {
        for (const auto &plugin : m_controller->pluginManager()->plugins()) {
            result.append(QString("%1 [%2] - %3 (%4)")
                          .arg(plugin.name())
                          .arg(plugin.statusString())
                          .arg(plugin.description())
                          .arg(plugin.id()));
        }
    }
    return result;
}

SpotlightAdaptor::SpotlightAdaptor(SearchController *controller)
    : QDBusAbstractAdaptor(controller)
    , m_controller(controller)
{
    connect(m_controller, &SearchController::windowVisibleChanged, this, [this]() {
        if (m_controller->isWindowVisible()) {
            emit Shown();
        } else {
            emit Hidden();
        }
    });
}

void SpotlightAdaptor::Show()
{
    m_controller->showWindow();
}

void SpotlightAdaptor::Hide()
{
    m_controller->hideWindow();
}

void SpotlightAdaptor::Toggle()
{
    m_controller->toggleWindow();
}

void SpotlightAdaptor::Search(const QString &query)
{
    m_controller->showWindow();
    m_controller->setQuery(query);
}

void SpotlightAdaptor::ReloadPlugins()
{
    m_controller->reloadPlugins();
}

QStringList SpotlightAdaptor::ListPlugins()
{
    QStringList result;
    if (m_controller->pluginManager()) {
        for (const auto &plugin : m_controller->pluginManager()->plugins()) {
            result.append(QString("%1 [%2] - %3 (%4)")
                          .arg(plugin.name())
                          .arg(plugin.statusString())
                          .arg(plugin.description())
                          .arg(plugin.id()));
        }
    }
    return result;
}
