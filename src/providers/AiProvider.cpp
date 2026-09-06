#include "AiProvider.h"
#include "../services/AiService.h"
#include "../services/ConfigService.h"
#include "../services/ClipboardService.h"
#include "../services/NotificationService.h"
#include "../services/UsageHistory.h"

AiProvider::AiProvider(QObject *parent)
    : SearchProvider(parent)
{
    connect(&AiService::instance(), &AiService::queryFinished, this, &AiProvider::onQueryFinished);
}

AiProvider::~AiProvider()
{
}

QList<SearchResult> AiProvider::search(const QString &query)
{
    QString trimmed = query.trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }

    bool isExplicitAi = trimmed.startsWith('>');
    bool isQuestion = trimmed.endsWith('?') ||
                      trimmed.startsWith("what ", Qt::CaseInsensitive) ||
                      trimmed.startsWith("how ", Qt::CaseInsensitive) ||
                      trimmed.startsWith("why ", Qt::CaseInsensitive) ||
                      trimmed.startsWith("who ", Qt::CaseInsensitive) ||
                      trimmed.startsWith("where ", Qt::CaseInsensitive) ||
                      trimmed.startsWith("explain ", Qt::CaseInsensitive) ||
                      trimmed.startsWith("define ", Qt::CaseInsensitive) ||
                      trimmed.contains("meaning", Qt::CaseInsensitive);

    if (!isExplicitAi && !isQuestion) {
        return {};
    }

    QString prompt = isExplicitAi ? trimmed.mid(1).trimmed() : trimmed;

    ConfigService &cfg = ConfigService::instance();
    QList<SearchResult> results;

    // Check if provider requires an API key but none is set
    bool needsKey = (cfg.aiProvider() != "ollama");
    if (needsKey && cfg.aiApiKey().isEmpty()) {
        if (isExplicitAi) {
            SearchResult sr;
            sr.setId("ai:config");
            sr.setTitle("Configure " + cfg.aiProviderName());
            sr.setSubtitle("API key is required. Press Enter to configure in Preferences.");
            sr.setIcon("dialog-password");
            sr.setScore(100.0);
            sr.setType("AI");
            sr.setProvider("AI Assistant");
            sr.setAction("open_preferences");
            results.append(sr);
            return results;
        }
        return {};
    }

    // Query is just ">"
    if (prompt.isEmpty()) {
        if (isExplicitAi) {
            SearchResult sr;
            sr.setId("ai:ready");
            sr.setTitle("AI Assistant · " + cfg.aiProviderName() + " (" + cfg.aiModel() + ")");
            sr.setSubtitle("Type your question after '>' and press Enter to ask");
            sr.setIcon("starred");
            sr.setScore(100.0);
            sr.setType("AI");
            sr.setProvider("AI Assistant");
            sr.setAction("none");
            results.append(sr);
            return results;
        }
        return {};
    }

    AiService &ai = AiService::instance();

    // Currently requesting AI
    if (ai.isRequesting() && ai.currentPrompt() == prompt) {
        SearchResult sr;
        sr.setId("ai:thinking");
        sr.setTitle("Thinking...");
        sr.setSubtitle("Consulting " + cfg.aiModel() + "... Please wait");
        sr.setIcon("view-refresh");
        sr.setScore(100.0);
        sr.setType("AI");
        sr.setProvider("AI Assistant");
        sr.setAction("none");
        results.append(sr);
        return results;
    }

    // Has response for prompt
    if (ai.hasResponseFor(prompt)) {
        SearchResult sr;
        sr.setId("ai:response");
        sr.setTitle(ai.lastResponse());
        sr.setSubtitle("AI Response · Press Enter to Copy · Esc to Dismiss");
        sr.setIcon("text-x-generic");
        sr.setScore(100.0);
        sr.setType("AI");
        sr.setProvider("AI Assistant");
        sr.setAction("copy_response");
        sr.setSecondaryAction("copy_response");
        sr.setSecondaryActionLabel("Copy Response");
        sr.setMetadataValue("response", ai.lastResponse());
        results.append(sr);
        return results;
    }

    // Has error for prompt
    if (ai.hasErrorFor(prompt)) {
        SearchResult sr;
        sr.setId("ai:error");
        sr.setTitle("AI Error: " + ai.lastError());
        sr.setSubtitle("Press Enter to retry or check Settings in Preferences");
        sr.setIcon("dialog-error");
        sr.setScore(100.0);
        sr.setType("AI");
        sr.setProvider("AI Assistant");
        sr.setAction("ask_ai");
        sr.setMetadataValue("prompt", prompt);
        results.append(sr);
        return results;
    }

    // Unsubmitted prompt suggestion
    SearchResult sr;
    sr.setId("ai:ask");
    sr.setTitle("Ask AI: \"" + prompt + "\"");
    sr.setSubtitle(isExplicitAi 
        ? ("Press Enter ↵ to query " + cfg.aiProviderName() + " (" + cfg.aiModel() + ")")
        : ("Press Enter ↵ to ask " + cfg.aiProviderName() + " (" + cfg.aiModel() + ") · Tip: Type > for AI mode"));
    sr.setIcon("starred");
    sr.setScore(isExplicitAi ? 100.0 : 45.0);
    sr.setType("AI");
    sr.setProvider("AI Assistant");
    sr.setAction("ask_ai");
    sr.setMetadataValue("prompt", prompt);
    results.append(sr);

    return results;
}

void AiProvider::searchAsync(quint64 requestId, const QString &query)
{
    // Synchronous results handle the immediate UI states
    Q_UNUSED(requestId);
    Q_UNUSED(query);
}

bool AiProvider::execute(const SearchResult &result, const QString &action)
{
    QString act = action.isEmpty() ? result.action() : action;

    if (act == "open_preferences") {
        emit openPreferencesRequested();
        return true;
    }

    if (act == "ask_ai") {
        QString prompt = result.metadataValue("prompt").toString();
        m_lastQueriedPrompt = prompt;
        AiService::instance().queryAsync(prompt);
        emit requestRefresh();
        return true;
    }

    if (act == "copy_response") {
        QString response = result.metadataValue("response").toString();
        if (response.isEmpty()) {
            response = AiService::instance().lastResponse();
        }
        ClipboardService::instance().setText(response);
        NotificationService::instance().notify("MVSpotlight AI", "AI response copied to clipboard!");
        UsageHistory::instance().recordLaunch("ai:response");
        return true;
    }

    if (act == "none") {
        return true;
    }

    return false;
}

void AiProvider::onQueryFinished(bool success, const QString &response, const QString &error)
{
    Q_UNUSED(success);
    Q_UNUSED(response);
    Q_UNUSED(error);
    emit requestRefresh();
}
