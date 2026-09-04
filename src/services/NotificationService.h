#pragma once

#include <QObject>
#include <QString>

class NotificationService : public QObject
{
    Q_OBJECT

public:
    static NotificationService& instance();

    Q_INVOKABLE void notify(const QString &title, const QString &body, const QString &icon = QString("dialog-information"));

private:
    explicit NotificationService(QObject *parent = nullptr);
};
