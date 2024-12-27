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
    QSqlDatabase database = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    database.setDatabaseName(DATABASE_FILE_NAME);

    if(!database.open()){
        qCritical() << "DB connection open error: " << database.lastError();
        return database;
    }
    else{
        qInfo() << "DB connection open success";
    }

    dbs[currentThread] = database;
    return database;
}
