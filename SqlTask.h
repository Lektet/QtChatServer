#ifndef SQLTASK_H
#define SQLTASK_H

#include <QObject>
#include <QRunnable>

#include <QSqlDatabase>

class SqlTask : public QObject, public QRunnable
{
    Q_OBJECT

public:
    SqlTask(int taskId);

private:
    int id;
    QString query;

    virtual QSqlQuery prepareQuery(QSqlDatabase db) = 0;
    virtual void finishTask(int id, QSqlQuery query) = 0;

    virtual void run() override;
};

#endif // SQLTASK_H
