#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include "../core/SearchResult.h"

class SearchProvider : public QObject
{
    Q_OBJECT

public:
    explicit SearchProvider(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~SearchProvider() = default;

    virtual QString id() const = 0;
    virtual QString name() const = 0;
    virtual bool isAsync() const { return false; }

    // Synchronous search for fast providers (<5ms)
    virtual QList<SearchResult> search(const QString &query) {
        Q_UNUSED(query);
        return {};
    }

    // Asynchronous search for providers that do disk/network/script operations
    virtual void searchAsync(quint64 requestId, const QString &query) {
        QList<SearchResult> res = search(query);
        emit resultsReady(requestId, res);
    }

    // Execute the result action
    virtual bool execute(const SearchResult &result, const QString &action = QString()) = 0;

signals:
    void resultsReady(quint64 requestId, const QList<SearchResult> &results);
};
