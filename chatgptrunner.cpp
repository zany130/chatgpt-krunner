#include "chatgptrunner.h"

#include <KRunner/Action>
#include <KLocalizedString>
#include <QClipboard>
#include <QDesktopServices>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QGuiApplication>
#include <QUrlQuery>

K_PLUGIN_CLASS_WITH_JSON(ChatGPTRunner, "chatgptrunner.json")

ChatGPTRunner::ChatGPTRunner(QObject *parent, const KPluginMetaData &metaData)
    : AbstractRunner(parent, metaData)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_pendingReply(nullptr)
    , m_debounceTimer(new QTimer(this))
{
    // Load configuration from environment variables
    m_apiBaseUrl = qEnvironmentVariable("GPT_KRUNNER_BASE_URL", "https://api.openai.com/v1");
    m_apiKey = qEnvironmentVariable("GPT_KRUNNER_API_KEY");
    m_model = qEnvironmentVariable("GPT_KRUNNER_MODEL", "gpt-4o-mini");
    
    // Setup debounce timer
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(DEBOUNCE_MS);
    connect(m_debounceTimer, &QTimer::timeout, this, &ChatGPTRunner::debounceTimeout);
    
    // Set match priority
    setPriority(AbstractRunner::NormalPriority);
    setSpeed(AbstractRunner::SlowSpeed);
    
    // Add actions
    KRunner::Action copyAction("copy", "edit-copy", i18n("Copy Answer"));
    copyAction.setData("copy");
    
    KRunner::Action openBrowserAction("browser", "internet-web-browser", i18n("Open in Browser"));
    openBrowserAction.setData("browser");
    
    m_actions = {copyAction, openBrowserAction};
}

ChatGPTRunner::~ChatGPTRunner()
{
    cancelPendingRequest();
}

void ChatGPTRunner::match(KRunner::RunnerContext &context)
{
    const QString query = context.query();
    
    // Check if query starts with "gpt "
    if (!query.startsWith(QLatin1String("gpt "), Qt::CaseInsensitive)) {
        return;
    }
    
    // Extract the actual query after "gpt "
    QString actualQuery = query.mid(4).trimmed();
    
    // If empty, show help
    if (actualQuery.isEmpty()) {
        KRunner::QueryMatch helpMatch(this);
        helpMatch.setType(KRunner::QueryMatch::HelperMatch);
        helpMatch.setIconName("help-about");
        helpMatch.setText(i18n("gpt <query> — Ask ChatGPT"));
        helpMatch.setSubtext(i18n("Type your question after 'gpt '"));
        helpMatch.setRelevance(1.0);
        context.addMatch(helpMatch);
        return;
    }
    
    // Check for API key
    if (m_apiKey.isEmpty()) {
        showError(context, i18n("Missing GPT_KRUNNER_API_KEY environment variable"));
        return;
    }
    
    // Check if we have cached answer
    QString cachedAnswer = getFromCache(actualQuery);
    if (!cachedAnswer.isEmpty()) {
        KRunner::QueryMatch match(this);
        match.setType(KRunner::QueryMatch::ExactMatch);
        match.setIconName("chatgpt");
        match.setText(cachedAnswer);
        match.setSubtext(i18n("Press Enter to copy • Actions: Copy / Open in browser"));
        match.setRelevance(1.0);
        match.setData(cachedAnswer);
        match.setActions(m_actions);
        context.addMatch(match);
        return;
    }
    
    // If query is too short, show waiting message
    if (actualQuery.length() < MIN_QUERY_LENGTH) {
        KRunner::QueryMatch match(this);
        match.setType(KRunner::QueryMatch::HelperMatch);
        match.setIconName("chatgpt");
        match.setText(i18n("Type at least %1 characters...", MIN_QUERY_LENGTH));
        match.setRelevance(0.5);
        context.addMatch(match);
        return;
    }
    
    // Store context and query
    m_lastContext = context;
    m_currentQuery = actualQuery;
    m_currentContextId = context.query(); // Use full query as context ID
    
    // Show placeholder immediately
    showPlaceholder(context, actualQuery);
    
    // Cancel any pending request
    cancelPendingRequest();
    
    // Start debounce timer
    m_debounceTimer->start();
}

void ChatGPTRunner::debounceTimeout()
{
    // User stopped typing, now make the API call
    if (!m_currentQuery.isEmpty() && !m_apiKey.isEmpty()) {
        queryLLM(m_currentQuery, m_currentContextId);
    }
}

void ChatGPTRunner::showPlaceholder(KRunner::RunnerContext &context, const QString &query)
{
    KRunner::QueryMatch match(this);
    match.setType(KRunner::QueryMatch::InformationalMatch);
    match.setIconName("chatgpt");
    match.setText(i18n("Thinking... \"%1\"", query.left(50)));
    match.setSubtext(i18n("Waiting for response from %1...", m_model));
    match.setRelevance(0.9);
    context.addMatch(match);
}

void ChatGPTRunner::showError(KRunner::RunnerContext &context, const QString &errorMsg)
{
    KRunner::QueryMatch match(this);
    match.setType(KRunner::QueryMatch::InformationalMatch);
    match.setIconName("dialog-error");
    match.setText(i18n("ChatGPT Runner Error"));
    match.setSubtext(errorMsg);
    match.setRelevance(1.0);
    context.addMatch(match);
}

void ChatGPTRunner::queryLLM(const QString &queryText, const QString &contextId)
{
    // Build the API request
    QUrl url(m_apiBaseUrl + "/chat/completions");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_apiKey).toUtf8());
    
    // Build JSON payload
    QJsonObject payload;
    payload["model"] = m_model;
    payload["max_tokens"] = 120;
    payload["temperature"] = 0.2;
    
    QJsonArray messages;
    QJsonObject systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = "Answer in <= 160 characters; no markdown; plain text.";
    messages.append(systemMsg);
    
    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = queryText;
    messages.append(userMsg);
    
    payload["messages"] = messages;
    
    QJsonDocument doc(payload);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    
    // Make the request
    m_pendingReply = m_networkManager->post(request, data);
    m_pendingReply->setProperty("query", queryText);
    m_pendingReply->setProperty("contextId", contextId);
    
    connect(m_pendingReply, &QNetworkReply::finished, this, &ChatGPTRunner::handleNetworkReply);
}

void ChatGPTRunner::handleNetworkReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }
    
    QString queryText = reply->property("query").toString();
    QString contextId = reply->property("contextId").toString();
    
    reply->deleteLater();
    m_pendingReply = nullptr;
    
    // Check for network errors
    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = i18n("Request failed: %1", reply->errorString());
        if (m_lastContext.isValid()) {
            showError(m_lastContext, errorMsg);
        }
        return;
    }
    
    // Parse JSON response
    QByteArray responseData = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    
    if (!doc.isObject()) {
        if (m_lastContext.isValid()) {
            showError(m_lastContext, i18n("Invalid JSON response"));
        }
        return;
    }
    
    QJsonObject obj = doc.object();
    
    // Check for API errors
    if (obj.contains("error")) {
        QJsonObject error = obj["error"].toObject();
        QString errorMsg = error["message"].toString();
        if (m_lastContext.isValid()) {
            showError(m_lastContext, i18n("API Error: %1", errorMsg));
        }
        return;
    }
    
    // Extract answer
    QJsonArray choices = obj["choices"].toArray();
    if (choices.isEmpty()) {
        if (m_lastContext.isValid()) {
            showError(m_lastContext, i18n("No response from API"));
        }
        return;
    }
    
    QJsonObject firstChoice = choices[0].toObject();
    QJsonObject message = firstChoice["message"].toObject();
    QString answer = message["content"].toString().trimmed();
    
    if (answer.isEmpty()) {
        if (m_lastContext.isValid()) {
            showError(m_lastContext, i18n("Empty response from API"));
        }
        return;
    }
    
    // Truncate to 160 chars if needed (for display)
    QString displayAnswer = answer;
    if (displayAnswer.length() > 160) {
        displayAnswer = displayAnswer.left(157) + "...";
    }
    
    // Add to cache
    addToCache(queryText, answer);
    
    // Create match with answer
    if (m_lastContext.isValid()) {
        KRunner::QueryMatch match(this);
        match.setType(KRunner::QueryMatch::ExactMatch);
        match.setIconName("chatgpt");
        match.setText(displayAnswer);
        match.setSubtext(i18n("Press Enter to copy • Actions: Copy / Open in browser"));
        match.setRelevance(1.0);
        match.setData(answer); // Store full answer
        match.setActions(m_actions);
        
        m_lastContext.addMatch(match);
    }
}

void ChatGPTRunner::run(const KRunner::RunnerContext &context, const KRunner::QueryMatch &match)
{
    Q_UNUSED(context)
    
    QString answer = match.data().toString();
    if (answer.isEmpty()) {
        return;
    }
    
    QString actionId = match.selectedAction() ? match.selectedAction().data().toString() : QString();
    
    if (actionId == "browser") {
        // Open ChatGPT in browser
        QDesktopServices::openUrl(QUrl("https://chatgpt.com"));
    } else {
        // Default action or "copy" action: copy to clipboard
        QClipboard *clipboard = QGuiApplication::clipboard();
        clipboard->setText(answer);
    }
}

void ChatGPTRunner::cancelPendingRequest()
{
    if (m_pendingReply) {
        m_pendingReply->abort();
        m_pendingReply->deleteLater();
        m_pendingReply = nullptr;
    }
}

void ChatGPTRunner::addToCache(const QString &query, const QString &answer)
{
    // If query already exists, remove it first (to update order)
    if (m_cache.contains(query)) {
        m_cacheOrder.removeAll(query);
    }
    
    // Add to cache
    m_cache[query] = answer;
    m_cacheOrder.enqueue(query);
    
    // Enforce max size
    while (m_cacheOrder.size() > MAX_CACHE_SIZE) {
        QString oldest = m_cacheOrder.dequeue();
        m_cache.remove(oldest);
    }
}

QString ChatGPTRunner::getFromCache(const QString &query)
{
    return m_cache.value(query, QString());
}

#include "chatgptrunner.moc"