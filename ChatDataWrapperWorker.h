#ifndef CHATDATAWRAPPERWORKER_H
#define CHATDATAWRAPPERWORKER_H

#include <QObject>

#include <QSqlDatabase>

#include "ChatMessageData.h"

#include <vector>
#include <queue>

#include <memory>

struct NewChatMessageData;
struct ChatMessageData;

class ChatDataRequest;

class ChatDataWrapperWorker : public QObject
{
    Q_OBJECT
public:
    explicit ChatDataWrapperWorker(QObject *parent = nullptr);

    int requestChatHistory();
    int requestAddChatMessage(const NewChatMessageData message);

    void start();
    void finish();

signals:
    void chatHistoryRequestCompleted(int id, const std::vector<ChatMessageData> history);
    void addChatMessageRequestCompleted(int id, bool result);

    void finished();

private:    
    QSqlDatabase dbConnection;

    std::queue<std::shared_ptr<ChatDataRequest>> requestQueue;
    int lastId;

    bool stopping;

    void continueProcessing();
    void processTopRequest();

    std::vector<ChatMessageData> getHistory();
    bool addMessage(const NewChatMessageData &message);
};

#endif // CHATDATAWRAPPERWORKER_H
