#pragma once

#include <QAbstractListModel>
#include <QList>
#include "SearchResult.h"

class SearchResultModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        TitleRole,
        SubtitleRole,
        IconRole,
        ScoreRole,
        TypeRole,
        ProviderRole,
        ActionRole,
        SecondaryActionRole,
        SecondaryActionLabelRole,
        MetadataRole
    };

    explicit SearchResultModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setResults(const QList<SearchResult> &results);
    void clear();

    Q_INVOKABLE QVariantMap get(int index) const;
    const SearchResult* resultAt(int index) const;

signals:
    void countChanged();

private:
    QList<SearchResult> m_results;
};
