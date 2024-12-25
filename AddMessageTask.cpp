#include "AddMessageTask.h"

#include <QSqlQuery>

#include <QDateTime>

AddChatMessageTask::AddChatMessageTask(int id, const NewChatMessageData& taskMessage) :
    SqlTask(id),
    message(taskMessage){

}

QSqlQuery AddChatMessageTask::prepareQuery(QSqlDatabase db){
    auto postTime = QString::number(QDateTime::currentMSecsSinceEpoch());
    QString queryString("INSERT INTO Messages ("
                        "Text, "
                        "PostTime, "
                        "UserId) "
                        "VALUES (\"%1\", \"%2\", (SELECT UserId FROM Users WHERE Username = \"%3\"));");
    queryString = queryString.arg(message.text)
                      .arg(postTime)
                      .arg(message.username);

    QSqlQuery newQuery(db);
    newQuery.prepare(queryString);
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
