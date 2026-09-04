#include "NotificationService.h"
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QVariantMap>
#include <QDebug>

NotificationService& NotificationService::instance()
{
    static NotificationService s_instance;
    return s_instance;
}

NotificationService::NotificationService(QObject *parent)
    : QObject(parent)
{
}

void NotificationService::notify(const QString &title, const QString &body, const QString &icon)
{
    QDBusInterface iface("org.freedesktop.Notifications",
                         "/org/freedesktop/Notifications",
                         "org.freedesktop.Notifications",
                         QDBusConnection::sessionBus());
    if (iface.isValid()) {
        QVariantMap hints;
        QStringList actions;
        iface.call("Notify",
                   QString("MVSpotlight"),
                   static_cast<quint32>(0),
                   icon.isEmpty() ? QString("dialog-information") : icon,
                   title,
                   body,
                   actions,
                   hints,
                   static_cast<qint32>(4000));
    } else {
        qWarning() << "Could not connect to org.freedesktop.Notifications:" << iface.lastError().message();
    }
}
