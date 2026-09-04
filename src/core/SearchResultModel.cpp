#include "SearchResultModel.h"

SearchResultModel::SearchResultModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int SearchResultModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_results.size();
}

QVariant SearchResultModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_results.size())
        return QVariant();

    const SearchResult &item = m_results.at(index.row());

    switch (role) {
    case IdRole:
        return item.id();
    case TitleRole:
        return item.title();
    case SubtitleRole:
        return item.subtitle();
    case IconRole:
        return item.icon();
    case ScoreRole:
        return item.score();
    case TypeRole:
        return item.type();
    case ProviderRole:
        return item.provider();
    case ActionRole:
        return item.action();
    case SecondaryActionRole:
        return item.secondaryAction();
    case SecondaryActionLabelRole:
        return item.secondaryActionLabel();
    case MetadataRole:
        return item.metadata();
    case Qt::DisplayRole:
        return item.title();
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> SearchResultModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[TitleRole] = "title";
    roles[SubtitleRole] = "subtitle";
    roles[IconRole] = "icon";
    roles[ScoreRole] = "score";
    roles[TypeRole] = "type";
    roles[ProviderRole] = "provider";
    roles[ActionRole] = "action";
    roles[SecondaryActionRole] = "secondaryAction";
    roles[SecondaryActionLabelRole] = "secondaryActionLabel";
    roles[MetadataRole] = "metadata";
    return roles;
}

void SearchResultModel::setResults(const QList<SearchResult> &results)
{
    beginResetModel();
    m_results = results;
    endResetModel();
    emit countChanged();
}

void SearchResultModel::clear()
{
    beginResetModel();
    m_results.clear();
    endResetModel();
    emit countChanged();
}

QVariantMap SearchResultModel::get(int index) const
{
    if (index < 0 || index >= m_results.size())
        return QVariantMap();

    const SearchResult &item = m_results.at(index);
    QVariantMap map;
    map["id"] = item.id();
    map["title"] = item.title();
    map["subtitle"] = item.subtitle();
    map["icon"] = item.icon();
    map["score"] = item.score();
    map["type"] = item.type();
    map["provider"] = item.provider();
    map["action"] = item.action();
    map["secondaryAction"] = item.secondaryAction();
    map["secondaryActionLabel"] = item.secondaryActionLabel();
    map["metadata"] = item.metadata();
    return map;
}

const SearchResult* SearchResultModel::resultAt(int index) const
{
    if (index < 0 || index >= m_results.size())
        return nullptr;
    return &m_results.at(index);
}
