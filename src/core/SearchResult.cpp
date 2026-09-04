#include "SearchResult.h"

SearchResult::SearchResult()
    : m_score(0.0)
{
}

SearchResult::SearchResult(const QString &id,
                           const QString &title,
                           const QString &subtitle,
                           const QString &icon,
                           double score,
                           const QString &type,
                           const QString &provider,
                           const QString &action,
                           const QVariantMap &metadata)
    : m_id(id)
    , m_title(title)
    , m_subtitle(subtitle)
    , m_icon(icon)
    , m_score(score)
    , m_type(type)
    , m_provider(provider)
    , m_action(action)
    , m_metadata(metadata)
{
}
