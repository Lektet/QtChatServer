#ifndef CHATDATAWRAPPER_H
#define CHATDATAWRAPPER_H

#include <QObject>

#include "ChatMessageData.h"

class ChatDataWrapperWorker;

struct ChatMessageData;
struct NewChatMessageData;

class ChatDataWrapper : public QObject
{
    Q_OBJECT

public:
    explicit ChatDataWrapper(QObject *parent = nullptr);

    int requestChatHistory();
    int requestAddChatMessage(const NewChatMessageData& message);

signals:
    void chatHistoryRequestCompleted(int id, const std::vector<ChatMessageData> history);
    void addChatMessageRequestCompleted(int id, bool result);

private:
    int lastId;
};

#endif // CHATDATAWRAPPER_H
