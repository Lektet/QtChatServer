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

    const QJsonArray getChatHistory() const;
    bool addChatMessage(const QJsonObject message);

signals:
    void messagesUpdated();

private:
    QJsonArray chatHistory;
    int maxIdValue;

    std::mutex writeMutex;

    //TODO: Write and read max id
    void readMessagesDataFromFile();
    bool writeMessagesDataToFile();
};

#endif // CHATDATAPROVIDER_H
