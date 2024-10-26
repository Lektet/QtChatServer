#ifndef CHATDATAPROVIDER_H
#define CHATDATAPROVIDER_H

#include <QObject>

#include <QSqlDatabase>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <future>
#include <mutex>

struct ChatMessageData;
struct NewChatMessageData;

class ChatDataProvider : public QObject
{
    Q_OBJECT

public:
    explicit ChatDataProvider(QObject *parent = nullptr);

    std::vector<ChatMessageData> getChatHistory() const;//TODO: Is thread safe?
    bool addChatMessage(const NewChatMessageData& message);//TODO: Check if making message a reference is possible

private:
    QSqlDatabase database;

//    QSqlDatabase  database();
};

#endif // CHATDATAPROVIDER_H
