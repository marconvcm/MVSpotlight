#include "AiService.h"
#include "ConfigService.h"
#include "MarkdownRenderer.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QElapsedTimer>
#include <QDebug>

AiService& AiService::instance()
{
    static AiService s_instance;
    return s_instance;
}

AiService::AiService(QObject *parent)
    : QObject(parent)
{
}

AiService::~AiService()
{
    cancelCurrentRequest();
}

void AiService::cancelCurrentRequest()
{
    if (m_activeReply) {
        m_activeReply->abort();
        m_activeReply->deleteLater();
        m_activeReply = nullptr;
    }
    if (m_isRequesting) {
        m_isRequesting = false;
        emit isRequestingChanged();
    }
}

void AiService::buildRequest(const QString &provider, const QString &apiKey, const QString &model,
                            const QString &endpoint, const QString &prompt, const QString &systemPrompt,
                            qreal temperature, int maxTokens, QNetworkRequest &request, QByteArray &body)
{
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    if (provider == "gemini") {
        QString urlStr = endpoint;
        if (urlStr.isEmpty()) {
            urlStr = QString("https://generativelanguage.googleapis.com/v1beta/models/%1:generateContent?key=%2")
                         .arg(model.isEmpty() ? "gemini-2.0-flash" : model, apiKey);
        } else if (!apiKey.isEmpty() && !urlStr.contains("key=")) {
            QUrl url(urlStr);
            QUrlQuery query(url);
            query.addQueryItem("key", apiKey);
            url.setQuery(query);
            urlStr = url.toString();
        }
        request.setUrl(QUrl(urlStr));

        QJsonObject rootObj;
        QJsonArray contentsArr;
        QJsonObject userTurn;
        userTurn["role"] = "user";
        QJsonArray partsArr;
        QJsonObject textPart;
        textPart["text"] = prompt;
        partsArr.append(textPart);
        userTurn["parts"] = partsArr;
        contentsArr.append(userTurn);
        rootObj["contents"] = contentsArr;

        QJsonObject genConfig;
        genConfig["temperature"] = temperature;
        genConfig["maxOutputTokens"] = maxTokens;
        rootObj["generationConfig"] = genConfig;

        if (!systemPrompt.isEmpty()) {
            QJsonObject sysInstruction;
            QJsonArray sysParts;
            QJsonObject sysText;
            sysText["text"] = systemPrompt;
            sysParts.append(sysText);
            sysInstruction["parts"] = sysParts;
            rootObj["systemInstruction"] = sysInstruction;
        }

        body = QJsonDocument(rootObj).toJson(QJsonDocument::Compact);
    }
    else if (provider == "claude") {
        QString urlStr = endpoint.isEmpty() ? "https://api.anthropic.com/v1/messages" : endpoint;
        request.setUrl(QUrl(urlStr));
        request.setRawHeader("x-api-key", apiKey.toUtf8());
        request.setRawHeader("anthropic-version", "2023-06-01");

        QJsonObject rootObj;
        rootObj["model"] = model.isEmpty() ? "claude-3-5-sonnet-20241022" : model;
        if (!systemPrompt.isEmpty()) {
            rootObj["system"] = systemPrompt;
        }

        QJsonArray messagesArr;
        QJsonObject userMsg;
        userMsg["role"] = "user";
        userMsg["content"] = prompt;
        messagesArr.append(userMsg);
        rootObj["messages"] = messagesArr;

        rootObj["max_tokens"] = maxTokens;
        rootObj["temperature"] = temperature;

        body = QJsonDocument(rootObj).toJson(QJsonDocument::Compact);
    }
    else {
        // OpenAI / Ollama / Custom (OpenAI-compatible format)
        QString urlStr = endpoint;
        if (urlStr.isEmpty()) {
            if (provider == "ollama") {
                urlStr = "http://localhost:11434/v1/chat/completions";
            } else {
                urlStr = "https://api.openai.com/v1/chat/completions";
            }
        }
        request.setUrl(QUrl(urlStr));

        if (!apiKey.isEmpty()) {
            request.setRawHeader("Authorization", ("Bearer " + apiKey).toUtf8());
        }

        QJsonObject rootObj;
        rootObj["model"] = model.isEmpty() ? (provider == "ollama" ? "llama3.2" : "gpt-4o-mini") : model;

        QJsonArray messagesArr;
        if (!systemPrompt.isEmpty()) {
            QJsonObject sysMsg;
            sysMsg["role"] = "system";
            sysMsg["content"] = systemPrompt;
            messagesArr.append(sysMsg);
        }

        QJsonObject userMsg;
        userMsg["role"] = "user";
        userMsg["content"] = prompt;
        messagesArr.append(userMsg);
        rootObj["messages"] = messagesArr;

        rootObj["temperature"] = temperature;
        rootObj["max_tokens"] = maxTokens;

        body = QJsonDocument(rootObj).toJson(QJsonDocument::Compact);
    }
}

bool AiService::parseResponse(const QString &provider, int statusCode, const QByteArray &data,
                             QString &outText, QString &outError)
{
    QJsonParseError parseErr;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseErr);

    if (parseErr.error != QJsonParseError::NoError) {
        if (statusCode >= 200 && statusCode < 300) {
            outText = QString::fromUtf8(data).trimmed();
            return !outText.isEmpty();
        }
        outError = QString("Invalid response format (HTTP %1)").arg(statusCode);
        return false;
    }

    QJsonObject root = doc.object();

    // Check for standard error payloads
    if (root.contains("error")) {
        QJsonValue errVal = root["error"];
        if (errVal.isObject()) {
            QJsonObject errObj = errVal.toObject();
            outError = errObj.value("message").toString();
            if (outError.isEmpty()) {
                outError = errObj.value("status").toString();
            }
        } else if (errVal.isString()) {
            outError = errVal.toString();
        }
        if (outError.isEmpty()) {
            outError = QString("API Error (HTTP %1)").arg(statusCode);
        }
        return false;
    }

    if (statusCode < 200 || statusCode >= 300) {
        outError = QString("HTTP %1 Error: %2").arg(statusCode).arg(QString::fromUtf8(data).left(120));
        return false;
    }

    // 1. Google Gemini response parsing
    if (provider == "gemini") {
        QJsonArray candidates = root["candidates"].toArray();
        if (!candidates.isEmpty()) {
            QJsonObject firstCand = candidates.first().toObject();
            QJsonObject content = firstCand["content"].toObject();
            QJsonArray parts = content["parts"].toArray();
            if (!parts.isEmpty()) {
                outText = parts.first().toObject().value("text").toString().trimmed();
                return true;
            }
        }
        outError = "Empty response from Gemini";
        return false;
    }

    // 2. Claude response parsing
    if (provider == "claude") {
        QJsonArray contentArr = root["content"].toArray();
        if (!contentArr.isEmpty()) {
            for (const auto &item : contentArr) {
                QJsonObject itemObj = item.toObject();
                if (itemObj.value("type").toString() == "text") {
                    outText = itemObj.value("text").toString().trimmed();
                    return true;
                }
            }
        }
        outError = "Empty response from Claude";
        return false;
    }

    // 3. OpenAI / Ollama / Custom response parsing
    QJsonArray choices = root["choices"].toArray();
    if (!choices.isEmpty()) {
        QJsonObject firstChoice = choices.first().toObject();
        QJsonObject message = firstChoice["message"].toObject();
        outText = message.value("content").toString().trimmed();
        return true;
    }

    // Fallback: check Ollama direct response key
    if (root.contains("response")) {
        outText = root["response"].toString().trimmed();
        return true;
    }

    outError = "Could not parse AI response";
    return false;
}

void AiService::queryAsync(const QString &prompt, QueryCallback callback)
{
    cancelCurrentRequest();

    ConfigService &cfg = ConfigService::instance();
    QString provider = cfg.aiProvider();
    QString apiKey = cfg.aiApiKey();
    QString model = cfg.aiModel();
    QString endpoint = cfg.aiEndpoint();
    QString systemPrompt = cfg.aiSystemPrompt();
    qreal temperature = cfg.aiTemperature();
    int maxTokens = cfg.aiMaxTokens();

    m_currentPrompt = prompt;
    m_lastResponse.clear();
    m_lastError.clear();
    m_isRequesting = true;
    emit isRequestingChanged();
    emit currentPromptChanged();
    emit lastResponseChanged();
    emit lastErrorChanged();

    QNetworkRequest request;
    QByteArray body;
    buildRequest(provider, apiKey, model, endpoint, prompt, systemPrompt, temperature, maxTokens, request, body);

    m_activeReply = m_nam.post(request, body);

    connect(m_activeReply, &QNetworkReply::finished, this, [this, provider, prompt, callback]() {
        if (!m_activeReply) return;

        int statusCode = m_activeReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QByteArray data = m_activeReply->readAll();
        QNetworkReply::NetworkError netErr = m_activeReply->error();
        m_activeReply->deleteLater();
        m_activeReply = nullptr;

        m_isRequesting = false;
        emit isRequestingChanged();

        QString text;
        QString err;

        if (netErr != QNetworkReply::NoError && netErr != QNetworkReply::ProtocolUnknownError) {
            // Check if server returned a JSON error payload despite error code
            if (!data.isEmpty() && parseResponse(provider, statusCode, data, text, err)) {
                m_lastResponse = text;
                emit lastResponseChanged();
                emit queryFinished(true, text, QString());
                if (callback) callback(true, text, QString());
                return;
            }
            err = QString("Network Error: %1 (HTTP %2)").arg(m_activeReply ? m_activeReply->errorString() : "Failed").arg(statusCode);
        }

        bool ok = parseResponse(provider, statusCode, data, text, err);
        if (ok) {
            m_lastResponse = text;
            m_lastError.clear();
            emit lastResponseChanged();
            emit lastErrorChanged();
            emit queryFinished(true, text, QString());
            if (callback) callback(true, text, QString());
        } else {
            m_lastError = err;
            m_lastResponse.clear();
            emit lastErrorChanged();
            emit lastResponseChanged();
            emit queryFinished(false, QString(), err);
            if (callback) callback(false, QString(), err);
        }
    });
}

void AiService::testConnection(const QString &provider, const QString &apiKey,
                              const QString &model, const QString &endpoint)
{
    if (m_testReply) {
        m_testReply->abort();
        m_testReply->deleteLater();
        m_testReply = nullptr;
    }

    QNetworkRequest request;
    QByteArray body;
    buildRequest(provider, apiKey, model, endpoint, "Hi! Test response only.", "Be brief.", 0.2, 32, request, body);

    auto timer = new QElapsedTimer();
    timer->start();

    m_testReply = m_nam.post(request, body);

    connect(m_testReply, &QNetworkReply::finished, this, [this, provider, model, timer]() {
        if (!m_testReply) {
            delete timer;
            return;
        }

        qint64 elapsed = timer->elapsed();
        delete timer;

        int statusCode = m_testReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QByteArray data = m_testReply->readAll();
        m_testReply->deleteLater();
        m_testReply = nullptr;

        QString text;
        QString err;
        bool ok = parseResponse(provider, statusCode, data, text, err);

        if (ok) {
            emit testConnectionFinished(true, QString("Connected to %1 (%2) in %3ms")
                                                 .arg(provider.toUpper(), model.isEmpty() ? "default" : model)
                                                 .arg(elapsed));
        } else {
            emit testConnectionFinished(false, QString("Connection failed: %1").arg(err));
        }
    });
}

QString AiService::renderMarkdown(const QString &markdown, bool isDark, const QString &accentColor) const
{
    return MarkdownRenderer::toHtml(markdown, isDark, accentColor);
}
