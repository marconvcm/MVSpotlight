#pragma once

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <functional>

class AiService : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool isRequesting READ isRequesting NOTIFY isRequestingChanged)
    Q_PROPERTY(QString currentPrompt READ currentPrompt NOTIFY currentPromptChanged)
    Q_PROPERTY(QString lastResponse READ lastResponse NOTIFY lastResponseChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    static AiService& instance();

    bool isRequesting() const { return m_isRequesting; }
    QString currentPrompt() const { return m_currentPrompt; }
    QString lastResponse() const { return m_lastResponse; }
    QString lastError() const { return m_lastError; }

    Q_INVOKABLE bool hasResponseFor(const QString &prompt) const {
        return !m_lastResponse.isEmpty() && m_currentPrompt == prompt;
    }

    Q_INVOKABLE bool hasErrorFor(const QString &prompt) const {
        return !m_lastError.isEmpty() && m_currentPrompt == prompt;
    }

    using QueryCallback = std::function<void(bool success, const QString &response, const QString &error)>;

    void queryAsync(const QString &prompt, QueryCallback callback = nullptr);

    Q_INVOKABLE void testConnection(const QString &provider, const QString &apiKey,
                                   const QString &model, const QString &endpoint);
    Q_INVOKABLE void cancelCurrentRequest();
    Q_INVOKABLE QString renderMarkdown(const QString &markdown, bool isDark = true, const QString &accentColor = "#8A2BE2") const;

signals:
    void isRequestingChanged();
    void currentPromptChanged();
    void lastResponseChanged();
    void lastErrorChanged();
    void testConnectionFinished(bool success, const QString &message);
    void queryFinished(bool success, const QString &response, const QString &error);

private:
    explicit AiService(QObject *parent = nullptr);
    ~AiService() override;

    void buildRequest(const QString &provider, const QString &apiKey, const QString &model,
                      const QString &endpoint, const QString &prompt, const QString &systemPrompt,
                      qreal temperature, int maxTokens, QNetworkRequest &request, QByteArray &body);

    bool parseResponse(const QString &provider, int statusCode, const QByteArray &data,
                       QString &outText, QString &outError);

    QNetworkAccessManager m_nam;
    QNetworkReply *m_activeReply{nullptr};
    QNetworkReply *m_testReply{nullptr};

    bool m_isRequesting{false};
    QString m_currentPrompt;
    QString m_lastResponse;
    QString m_lastError;
};
