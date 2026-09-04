#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include "../core/SearchResult.h"

struct SystemActionItem {
    QString id;
    QString title;
    QString subtitle;
    QString icon;
    QStringList keywords;
    QString command;
    QStringList args;
};

class SystemActions : public QObject
{
    Q_OBJECT

public:
    static SystemActions& instance();

    QList<SearchResult> search(const QString &query) const;
    bool execute(const QString &actionId);

private:
    explicit SystemActions(QObject *parent = nullptr);
    void initActions();

    QList<SystemActionItem> m_actions;
};
