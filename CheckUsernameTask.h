#ifndef CHECKUSERNAMETASK_H
#define CHECKUSERNAMETASK_H

#include "SqlTask.h"

class CheckUsernameTask : public SqlTask
{
    Q_OBJECT
public:
    CheckUsernameTask(int id, const QString& checkedUsername);

signals:
    void taskCompleted(int id, bool isValid);

private:
    QString username;

    virtual QSqlQuery prepareQuery(QSqlDatabase db) override;
    virtual void finishTask(int id, QSqlQuery query) override;
};

#endif // CHECKUSERNAMETASK_H
