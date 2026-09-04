#pragma once

#include "SearchProvider.h"
#include <optional>

class CalculatorProvider : public SearchProvider
{
    Q_OBJECT

public:
    explicit CalculatorProvider(QObject *parent = nullptr);

    QString id() const override { return "calculator"; }
    QString name() const override { return "Calculator"; }

    QList<SearchResult> search(const QString &query) override;
    bool execute(const SearchResult &result, const QString &action = QString()) override;

    static std::optional<double> evaluate(const QString &expression);

private:
    static bool isMathExpression(const QString &query);
};
