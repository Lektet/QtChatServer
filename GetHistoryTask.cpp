#include "GetHistoryTask.h"

#include <QSqlQuery>
#include <QSqlRecord>

#include <QVariant>

const QString MESSAGE_ID_FIELD = "MessageId";
const QString MESSAGE_USERNAME_FIELD = "Username";
const QString MESSAGE_TEXT_FIELD = "Text";
const QString MESSAGE_POST_TIME_FIELD = "PostTime";

GetHistoryTask::GetHistoryTask(int id) :
    SqlTask(id){

}

QSqlQuery GetHistoryTask::prepareQuery(QSqlDatabase db){
    QString queryString("SELECT %1, %2, %3, %4 "
                        "FROM Messages INNER JOIN Users "
                        "ON Messages.UserId = Users.UserId;");
    queryString = queryString.arg(MESSAGE_ID_FIELD).
                  arg(MESSAGE_USERNAME_FIELD)
                      .arg(MESSAGE_TEXT_FIELD)
                      .arg(MESSAGE_POST_TIME_FIELD);

    QSqlQuery newQuery(db);
    bool prepareResult = newQuery.prepare(queryString);
    return newQuery;
}

void GetHistoryTask::finishTask(int id, QSqlQuery query){
    if(!query.isActive()){
        emit taskCompleted(id, std::vector<ChatMessageData>());
        return;
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

    emit taskCompleted(id, messages);
}
