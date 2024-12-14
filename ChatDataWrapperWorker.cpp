#include "ChatDataWrapperWorker.h"

#include <QThread>

#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include <QDateTime>

#include <GetHistoryRequest.h>
#include <AddMessageRequest.h>

#include <ChatDataRequestType.h>

#include "NewChatMessageData.h"

#include <QDebug>

const QString MESSAGE_ID_FIELD = "MessageId";
const QString MESSAGE_USERNAME_FIELD = "Username";
const QString MESSAGE_TEXT_FIELD = "Text";
const QString MESSAGE_POST_TIME_FIELD = "PostTime";

const QString DATABASE_FILE_NAME = "database";

ChatDataWrapperWorker::ChatDataWrapperWorker(QObject *parent)
    : QObject{parent},
      lastId(0),
      stopping(false)
{

}

int ChatDataWrapperWorker::requestChatHistory()
{
    requestQueue.push(std::make_shared<GetHistoryRequest>(++lastId));

    continueProcessing();
    return lastId;
}

int ChatDataWrapperWorker::requestAddChatMessage(const NewChatMessageData message)
{
    requestQueue.push(std::make_shared<AddMessageRequest>(++lastId, message));

    continueProcessing();
    return lastId;
}

void ChatDataWrapperWorker::start()
{
    auto connectionName = QString::number((long long)QThread::currentThreadId(), 16);
    dbConnection = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    dbConnection.setDatabaseName(DATABASE_FILE_NAME);
    auto ok = dbConnection.open();
    if(!ok){
        qWarning() << "Open database error: " << dbConnection.lastError();
    }
    else{
        qInfo() << "Open database success";
    }
}

void ChatDataWrapperWorker::finish()
{
    stopping = true;

    emit finished();
}

void ChatDataWrapperWorker::continueProcessing()
{
    if(stopping){
        return;
    }

    QMetaObject::invokeMethod(this,
                              &ChatDataWrapperWorker::processTopRequest,
                              Qt::QueuedConnection);

}

void ChatDataWrapperWorker::processTopRequest()
{
    if(requestQueue.empty()){
        return;
    }

    auto request = requestQueue.front();
    requestQueue.pop();

    switch (request->getActionType()) {
    case ChatDataRequestType::GetHistory:
    {
        auto history = getHistory();
        emit chatHistoryRequestCompleted(request->getActionId(), history);
        break;
    }
    case ChatDataRequestType::AddMessage:
    {
        auto addMessageRequest = std::static_pointer_cast<AddMessageRequest>(request);
        auto result = addMessage(addMessageRequest->getMessageData());
        emit addChatMessageRequestCompleted(addMessageRequest->getActionId(), result);
        break;
    }
    default:
        break;
    }

    continueProcessing();
}

std::vector<ChatMessageData> ChatDataWrapperWorker::getHistory()
{
    QSqlQuery query(dbConnection);
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

bool ChatDataWrapperWorker::addMessage(const NewChatMessageData &message)
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
    QSqlQuery query(dbConnection);
    QString queryString("INSERT INTO Messages ("
                        "Text, "
                        "PostTime, "
                        "UserId) "
                        "VALUES (\"%1\", \"%2\", (SELECT UserId FROM Users WHERE Username = \"%3\"));");
    queryString = queryString.arg(message.text)
            .arg(postTime)
            .arg(message.username);
    query.prepare(queryString);

    auto execResult = query.exec();
    if(!execResult){
        qWarning() << "Adding message to db failed: " << query.lastError();
        return false;
    }
    return true;
}
