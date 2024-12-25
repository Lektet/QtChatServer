#ifndef SQLCONNECTIONMANAGER_H
#define SQLCONNECTIONMANAGER_H

#include <QSqlDatabase>
#include <QThread>

#include <map>

class SqlConnectionManager
{
public:
    static SqlConnectionManager& getManager()
    {
        static SqlConnectionManager instance;
        return instance;
    }

    QSqlDatabase getDb();

    SqlConnectionManager(SqlConnectionManager const&) = delete;
    void operator=(SqlConnectionManager const&) = delete;

private:
    SqlConnectionManager() {};

    std::map<QThread*, QSqlDatabase> dbs;
    std::mutex dbsMutex;
};

#endif // SQLCONNECTIONMANAGER_H
