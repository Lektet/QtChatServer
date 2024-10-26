#include "ChatDataProvider.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>

#include <QJsonObject>
#include <QJsonArray>

#include <QDateTime>
#include <QFile>

#include <QThread>

#include "ChatMessageData.h"
#include "NewChatMessageData.h"

#include <QDebug>

const QString MESSAGE_ID_FIELD = "MessageId";
const QString MESSAGE_USERNAME_FIELD = "Username";
const QString MESSAGE_TEXT_FIELD = "Text";
const QString MESSAGE_POST_TIME_FIELD = "PostTime";

const QString DATABASE_FILE_NAME = "database";

ChatDataProvider::ChatDataProvider(QObject *parent) :
    QObject(parent)
{
    auto newDatabaseName = QString::number((long long)QThread::currentThreadId(), 16);
    database = QSqlDatabase::addDatabase("QSQLITE", newDatabaseName);
    database.setDatabaseName(DATABASE_FILE_NAME);
    auto ok = database.open();
    if(!ok){
        qWarning() << "Open database error: " << database.lastError();

    }
    else{
        qInfo() << "Open database success";
    }
}

std::vector<ChatMessageData> ChatDataProvider::getChatHistory() const
{
    QSqlQuery query(database);
    QString queryString("SELECT %1, %2, %3, %4 "
                        "FROM Messages INNER JOIN Users "
                        "ON Messages.UserId = Users.UserId;");
    queryString = queryString.arg(MESSAGE_ID_FIELD).
            arg(MESSAGE_USERNAME_FIELD)
            .arg(MESSAGE_TEXT_FIELD)
            .arg(MESSAGE_POST_TIME_FIELD);
    query.prepare(queryString);
    if(!query.exec()){
        qDebug() << "Retrieving messages from db failed: " << query.lastError();
        return std::vector<ChatMessageData>();
    }

    auto record = query.record();
    auto messageIdIndex = record.indexOf(MESSAGE_ID_FIELD);
    auto usernameIndex = record.indexOf(MESSAGE_USERNAME_FIELD);
    auto textIndex = record.indexOf(MESSAGE_TEXT_FIELD);
    auto postTimeIndex = record.indexOf(MESSAGE_POST_TIME_FIELD);
    std::vector<ChatMessageData> messages;
    while(query.next()){
        ChatMessageData messageData;
        messageData.id = query.value(messageIdIndex).toString();
        messageData.username = query.value(usernameIndex).toString();
        messageData.text = query.value(textIndex).toString();
        messageData.postTime = query.value(postTimeIndex).toString();
        messages.push_back(std::move(messageData));
    }

    return messages;
}

bool ChatDataProvider::addChatMessage(const NewChatMessageData &message)
{
    if(message.text.isEmpty()){
        qWarning() << "Text is empty";
        return false;
    }

    if(message.username.isEmpty()){
        qWarning() << "Username is empty";
        return false;
    }

    auto postTime = QString::number(QDateTime::currentMSecsSinceEpoch());
    QSqlQuery query(database);
    QString queryString("INSERT INTO Messages ("
                        "Text, "
                        "PostTime, "
                        "UserId) "
                        "VALUES (\"%1\", \"%2\", (SELECT UserId FROM Users WHERE Username = \"%3\"));");
    queryString = queryString.arg(message.text)
            .arg(postTime)
            .arg(message.username);
    query.prepare(queryString);
    qDebug() << "Add message query: " << queryString;

    auto execResult = query.exec();
    if(!execResult){
        qWarning() << "Adding message to db failed: " << query.lastError();
        return false;
    }
    return true;
}
