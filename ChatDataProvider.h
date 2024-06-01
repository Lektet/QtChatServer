#ifndef CHATDATAPROVIDER_H
#define CHATDATAPROVIDER_H

#include <QObject>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <mutex>

class ChatDataProvider : public QObject
{
    Q_OBJECT

public:
    explicit ChatDataProvider(QObject *parent = nullptr);

    const QJsonArray getChatHistory() const;//TODO: Is thread safe?
    bool addChatMessage(const QJsonObject message);

signals:
    void messagesUpdated();

private:
    QJsonArray chatHistory;
    int maxIdValue;

    mutable std::mutex accessMutex;

    //TODO: Write and read max id
    void readMessagesDataFromFile();
    bool writeMessagesDataToFile();
};

#endif // CHATDATAPROVIDER_H
