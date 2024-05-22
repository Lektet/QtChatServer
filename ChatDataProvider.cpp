#include "ChatDataProvider.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QFile>

#include <QDebug>

const QString MESSAGE_USERNAME_KEY = "Username";
const QString MESSAGE_TEXT_KEY = "Text";
const QString MESSAGE_ID_KEY = "Id";
const QString MESSAGE_TIME_KEY = "Time";
const QString MESSAGES_FILENAME = "messagesData.json";
const QString MESSAGES_KEY = "Messages";
const QString MESSAGE_MAX_ID_KEY = "MaxId";

ChatDataProvider::ChatDataProvider(QObject *parent) :
    QObject(parent),
    maxIdValue(0)
{
    readMessagesDataFromFile();
}

const QJsonArray ChatDataProvider::getChatHistory() const
{
    return chatHistory;
}

bool ChatDataProvider::addChatMessage(const QJsonObject message)
{
    if(!message.contains(MESSAGE_USERNAME_KEY)){
        qDebug() << "Message have username";
        return false;
    }
    if(message.value(MESSAGE_USERNAME_KEY).toString().isEmpty()){
        qDebug() << "Username is empty";
        return false;
    }

    if(!message.contains(MESSAGE_TEXT_KEY)){
        qDebug() << "Message have text";
        return false;
    }
    if(message.value(MESSAGE_TEXT_KEY).toString().isEmpty()){
        qDebug() << "Message is empty";
        return false;
    }

    QJsonObject messageToAdd(message);
    messageToAdd.insert(MESSAGE_TIME_KEY, QDateTime::currentMSecsSinceEpoch());
    const std::lock_guard<std::mutex> lock(writeMutex);
    messageToAdd.insert(MESSAGE_ID_KEY, maxIdValue++);
    chatHistory.append(messageToAdd);

    auto writeResult = writeMessagesDataToFile();
    if(writeResult){
        emit messagesUpdated();
    }
    return writeResult;
}

void ChatDataProvider::readMessagesDataFromFile()
{
    QFile file(MESSAGES_FILENAME);
    if(!file.open(QFile::ReadOnly)){
        qDebug() << "messages file open error: " << file.errorString();
        return;
    }

    QJsonParseError parseError;
    auto document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if(document.isNull()){
        qDebug() << "Error parsing messages file: " << parseError.errorString();
        file.close();
        return;
    }
    file.close();

    auto jsonObject = document.object();
    if(!jsonObject.contains(MESSAGES_KEY)){
        qDebug() << "Can't find messages in file";
        return;
    }

    if(!jsonObject.contains(MESSAGE_MAX_ID_KEY)){
        qDebug() << "Can't find message max id in file";
        return;
    }

    auto messagesDataValue = jsonObject.value(MESSAGES_KEY);
    if(!messagesDataValue.isArray()){
        qDebug() << "Messages have wrong type";
        return;
    }

    auto maxIdDataValue = jsonObject.value(MESSAGE_MAX_ID_KEY);
    if(!maxIdDataValue.isDouble()){
        qDebug() << "Message max id have wrong type";
        return;
    }

    chatHistory = messagesDataValue.toArray();
    maxIdValue = maxIdDataValue.toInt();
}

bool ChatDataProvider::writeMessagesDataToFile()
{
    QFile file(MESSAGES_FILENAME);
    if(!file.open(QFile::WriteOnly)){
        qDebug() << "messages file open error: " << file.errorString();
        return false;
    }

    QJsonObject object;
    object.insert(MESSAGES_KEY, chatHistory);
    object.insert(MESSAGE_MAX_ID_KEY, maxIdValue);
    QJsonDocument document;
    document.setObject(object);
    if(file.write(document.toJson()) == -1){
        qDebug() << "Write chat history to file error";
        file.close();
        return false;
    }

    file.close();

    return true;
}
