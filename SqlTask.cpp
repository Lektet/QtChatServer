#include "SqlTask.h"

#include <QSqlQuery>
#include <QSqlError>

#include "SqlConnectionManager.h"

#include <QDebug>

SqlTask::SqlTask(int taskId):
    QRunnable(),
    id(taskId){

}

void SqlTask::run(){
    auto conn = SqlConnectionManager::getManager().getDb();
    if(!conn.isOpen()){
        finishTask(id, std::move(QSqlQuery()));
        return;
    }

    auto query = prepareQuery(conn);
    if(query.exec()){
        qInfo() << "Sql query exec success!";
    }
    else{
        qWarning() << "Sql quey exec failed: " << query.lastError();;
    }
    finishTask(id, std::move(query));
}
