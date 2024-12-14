#ifndef CHATDATAWRAPPER_H
#define CHATDATAWRAPPER_H

#include <QObject>

#include <QThread>

#include <QSqlDatabase>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "ChatMessageData.h"

#include <future>
#include <mutex>

class ChatDataWrapperWorker;

struct ChatMessageData;
struct NewChatMessageData;

class ChatDataWrapper : public QObject
{
    Q_OBJECT

public:
    explicit ChatDataWrapper(QObject *parent = nullptr);

    int requestChatHistory() const;//TODO: Is thread safe?
    int requestAddChatMessage(const NewChatMessageData& message);//TODO: Check if making message a reference is possible

signals:
    void chatHistoryRequestCompleted(int id, const std::vector<ChatMessageData> history);
    void addChatMessageRequestCompleted(int id, bool result);

private:
    QThread* workerThread;
    ChatDataWrapperWorker* worker;
};

#endif // CHATDATAWRAPPER_H
