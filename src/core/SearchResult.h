#pragma once

#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QMetaType>

class SearchResult
{
public:
    SearchResult();
    SearchResult(const QString &id,
                 const QString &title,
                 const QString &subtitle,
                 const QString &icon,
                 double score,
                 const QString &type,
                 const QString &provider,
                 const QString &action = QString(),
                 const QVariantMap &metadata = QVariantMap());

    QString id() const { return m_id; }
    void setId(const QString &id) { m_id = id; }

    QString title() const { return m_title; }
    void setTitle(const QString &title) { m_title = title; }

    QString subtitle() const { return m_subtitle; }
    void setSubtitle(const QString &subtitle) { m_subtitle = subtitle; }

    QString icon() const { return m_icon; }
    void setIcon(const QString &icon) { m_icon = icon; }

    double score() const { return m_score; }
    void setScore(double score) { m_score = score; }

    QString type() const { return m_type; }
    void setType(const QString &type) { m_type = type; }

    QString provider() const { return m_provider; }
    void setProvider(const QString &provider) { m_provider = provider; }

    QString action() const { return m_action; }
    void setAction(const QString &action) { m_action = action; }

    QVariantMap metadata() const { return m_metadata; }
    void setMetadata(const QVariantMap &metadata) { m_metadata = metadata; }
    void setMetadataValue(const QString &key, const QVariant &value) { m_metadata.insert(key, value); }
    QVariant metadataValue(const QString &key, const QVariant &defaultValue = QVariant()) const {
        return m_metadata.value(key, defaultValue);
    }

    QString secondaryActionLabel() const { return m_secondaryActionLabel; }
    void setSecondaryActionLabel(const QString &label) { m_secondaryActionLabel = label; }

    QString secondaryAction() const { return m_secondaryAction; }
    void setSecondaryAction(const QString &action) { m_secondaryAction = action; }

    bool isValid() const { return !m_id.isEmpty() && !m_title.isEmpty(); }

    bool operator==(const SearchResult &other) const {
        return m_id == other.m_id;
    }

private:
    QString m_id;
    QString m_title;
    QString m_subtitle;
    QString m_icon;
    double m_score{0.0};
    QString m_type;
    QString m_provider;
    QString m_action;
    QString m_secondaryActionLabel;
    QString m_secondaryAction;
    QVariantMap m_metadata;
};

Q_DECLARE_METATYPE(SearchResult)
