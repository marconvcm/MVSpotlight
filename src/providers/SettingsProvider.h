#pragma once

#include "SearchProvider.h"

struct SettingsPanel {
    QString id;
    QString name;
    QString panel;
    QString icon;
    QStringList keywords;
};

class SettingsProvider : public SearchProvider
{
    Q_OBJECT

public:
    explicit SettingsProvider(QObject *parent = nullptr);

    QString id() const override { return "settings"; }
    QString name() const override { return "GNOME Settings"; }

    QList<SearchResult> search(const QString &query) override;
    bool execute(const SearchResult &result, const QString &action = QString()) override;

private:
    void initPanels();
    QList<SettingsPanel> m_panels;
};
