#include "SqlConnectionManager.h"

#include <QSqlError>

#include <QDebug>

const QString DATABASE_FILE_NAME = "database";

QSqlDatabase SqlConnectionManager::getDb()
{
    std::lock_guard<std::mutex> lock(dbsMutex);

    auto currentThread = QThread::currentThread();
    if(dbs.contains(currentThread)){
        return dbs.at(currentThread);
    }

    auto connectionName = QString::number((long long)QThread::currentThreadId(), 16);
    QSqlDatabase dbConnection = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    dbConnection.setDatabaseName(DATABASE_FILE_NAME);

    if(!dbConnection.open()){
        qCritical() << "DB connection open error: " << dbConnection.lastError();
        return dbConnection;
    }
    else{
        qInfo() << "DB connection open success";
    }

    dbs[currentThread] = dbConnection;
    return dbConnection;
}
