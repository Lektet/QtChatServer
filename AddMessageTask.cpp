#include "AddMessageTask.h"

#include <QSqlQuery>

#include <QDateTime>
#include <QVariant>

AddChatMessageTask::AddChatMessageTask(int id, const NewChatMessageData& taskMessage) :
    SqlTask(id),
    message(taskMessage){

}

QSqlQuery AddChatMessageTask::prepareQuery(QSqlDatabase db){
    QSqlQuery newQuery(db);
    auto postTime = QString::number(QDateTime::currentMSecsSinceEpoch());
    newQuery.prepare("INSERT INTO Messages ("
                        "Text, "
                        "PostTime, "
                        "UserId) "
                        "VALUES (:text, :postTime, (SELECT UserId FROM Users WHERE Username = :username));");
    newQuery.bindValue(":text", QVariant(message.text));
    newQuery.bindValue(":postTime", postTime);
    newQuery.bindValue(":username", QVariant(message.username));
    return newQuery;
}

void AddChatMessageTask::finishTask(int id, QSqlQuery query){
    if(query.isActive()){
        emit taskCompleted(id, true);
    }
    else{
        emit taskCompleted(id, false);
    }
}
