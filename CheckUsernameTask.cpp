#include "CheckUsernameTask.h"

#include <QSqlQuery>
#include <QSqlRecord>

CheckUsernameTask::CheckUsernameTask(int id, const QString &checkedUsername) :
    SqlTask(id),
    username(checkedUsername)
{

}

QSqlQuery CheckUsernameTask::prepareQuery(QSqlDatabase db)
{
    QSqlQuery newQuery(db);
    newQuery.prepare("SELECT EXISTS("
                     "SELECT UserId FROM Users WHERE Users.Username = :username)"
                     "AS UserIsValid;");
    newQuery.bindValue(":username", username);
    return newQuery;
}

void CheckUsernameTask::finishTask(int id, QSqlQuery query)
{
    if(!query.isActive()){
        emit taskCompleted(id, false);
        return;
    }

    auto record = query.record();
    query.next();
    emit taskCompleted(id, query.value("UserIsValid").toBool());
}
