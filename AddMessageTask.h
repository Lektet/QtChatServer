#ifndef ADDMESSAGETASK_H
#define ADDMESSAGETASK_H

#include "SqlTask.h"

#include "NewChatMessageData.h"

class AddChatMessageTask : public SqlTask
{
    Q_OBJECT
public:
    AddChatMessageTask(int id, const NewChatMessageData& taskMessage);

signals:
    void taskCompleted(int id, bool result);

private:
    NewChatMessageData message;

    virtual QSqlQuery prepareQuery(QSqlDatabase db) override;
    virtual void finishTask(int id, QSqlQuery query) override;
};

#endif // ADDMESSAGETASK_H
