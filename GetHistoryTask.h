#ifndef GETHISTORYTASK_H
#define GETHISTORYTASK_H

#include "SqlTask.h"

#include "ChatMessageData.h"

class GetHistoryTask : public SqlTask
{
    Q_OBJECT

public:
    GetHistoryTask(int id);

signals:
    void taskCompleted(int id, const std::vector<ChatMessageData>& history);

private:
    virtual QSqlQuery prepareQuery(QSqlDatabase db) override;
    virtual void finishTask(int id, QSqlQuery query) override;
};

#endif // GETHISTORYTASK_H
