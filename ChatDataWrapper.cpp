#include "ChatDataWrapper.h"

#include <QThreadPool>

#include "ChatDataWrapperWorker.h"

#include "CheckUsernameTask.h"
#include "GetHistoryTask.h"
#include "AddMessageTask.h"

ChatDataWrapper::ChatDataWrapper(QObject *parent) :
    QObject(parent),
    lastId(0)
{
}

int ChatDataWrapper::requestChatHistory()
{
    auto task = new GetHistoryTask(++lastId);
    connect(task, &GetHistoryTask::taskCompleted,
            this, &ChatDataWrapper::chatHistoryRequestCompleted,
            Qt::QueuedConnection);
    QThreadPool::globalInstance()->start(task);
    return lastId;
}

int ChatDataWrapper::requestAddChatMessage(const NewChatMessageData &message)
{
    auto task = new AddChatMessageTask(++lastId, message);
    connect(task, &AddChatMessageTask::taskCompleted,
            this, &ChatDataWrapper::addChatMessageRequestCompleted,
            Qt::QueuedConnection);
    QThreadPool::globalInstance()->start(task);
    return lastId;
}

int ChatDataWrapper::requestCheckUsername(const QString &username)
{
    auto task = new CheckUsernameTask(++lastId, username);
    connect(task, &CheckUsernameTask::taskCompleted,
            this, &ChatDataWrapper::checkUsernameRequestCompleted,
            Qt::QueuedConnection);
    QThreadPool::globalInstance()->start(task);
    return lastId;
}
