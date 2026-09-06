#pragma once

#include "SearchProvider.h"

class AiProvider : public SearchProvider
{
    Q_OBJECT

public:
    explicit AiProvider(QObject *parent = nullptr);
    ~AiProvider() override;

    QString id() const override { return "ai"; }
    QString name() const override { return "AI Assistant"; }
    bool isAsync() const override { return false; }

    QList<SearchResult> search(const QString &query) override;
    void searchAsync(quint64 requestId, const QString &query) override;
    bool execute(const SearchResult &result, const QString &action = QString()) override;

signals:
    void openPreferencesRequested();
    void requestRefresh();

private slots:
    void onQueryFinished(bool success, const QString &response, const QString &error);

private:
    QString m_lastQueriedPrompt;
};
