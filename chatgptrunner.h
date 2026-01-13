#ifndef CHATGPTRUNNER_H
#define CHATGPTRUNNER_H

#include <KRunner/AbstractRunner>
#include <QNetworkAccessManager>
#include <QTimer>
#include <QHash>
#include <QQueue>

class ChatGPTRunner : public KRunner::AbstractRunner
{
    Q_OBJECT

public:
    ChatGPTRunner(QObject *parent, const KPluginMetaData &metaData);
    ~ChatGPTRunner() override;

    void match(KRunner::RunnerContext &context) override;
    void run(const KRunner::RunnerContext &context, const KRunner::QueryMatch &match) override;

private slots:
    void handleNetworkReply();
    void debounceTimeout();

private:
    void queryLLM(const QString &queryText, const QString &contextId);
    void showPlaceholder(KRunner::RunnerContext &context, const QString &query);
    void showError(KRunner::RunnerContext &context, const QString &errorMsg);
    void addToCache(const QString &query, const QString &answer);
    QString getFromCache(const QString &query);
    void cancelPendingRequest();

    QNetworkAccessManager *m_networkManager;
    QNetworkReply *m_pendingReply;
    QTimer *m_debounceTimer;
    
    // Cache: query -> answer (LRU with max 20 entries)
    QHash<QString, QString> m_cache;
    QQueue<QString> m_cacheOrder;
    static const int MAX_CACHE_SIZE = 20;
    
    // Current query tracking
    QString m_currentQuery;
    QString m_currentContextId;
    KRunner::RunnerContext m_lastContext;
    
    // Configuration from env vars
    QString m_apiBaseUrl;
    QString m_apiKey;
    QString m_model;
    
    // Actions
    QList<KRunner::Action> m_actions;
    
    static constexpr int DEBOUNCE_MS = 350;
    static constexpr int MIN_QUERY_LENGTH = 3;
};

#endif // CHATGPTRUNNER_H