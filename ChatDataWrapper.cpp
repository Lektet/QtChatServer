#include "ChatDataWrapper.h"

#include <QCoreApplication>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>

#include <QJsonObject>
#include <QJsonArray>

#include <QDateTime>
#include <QFile>

#include <QThread>

#include "ChatDataWrapperWorker.h"

#include "NewChatMessageData.h"

#include <QDebug>

ChatDataWrapper::ChatDataWrapper(QObject *parent) :
    QObject(parent),
    workerThread(new QThread(this)),
    worker(new ChatDataWrapperWorker)
{
    worker->moveToThread(workerThread);
    connect(worker, &ChatDataWrapperWorker::chatHistoryRequestCompleted,
            this, &ChatDataWrapper::chatHistoryRequestCompleted,
            Qt::QueuedConnection);
    connect(worker, &ChatDataWrapperWorker::addChatMessageRequestCompleted,
            this, &ChatDataWrapper::addChatMessageRequestCompleted,
            Qt::QueuedConnection);

    connect(qApp, &QCoreApplication::aboutToQuit, worker,
            &ChatDataWrapperWorker::finish, Qt::QueuedConnection);
    connect(worker, &ChatDataWrapperWorker::finished,
            workerThread, &QThread::quit);
    connect(worker, &ChatDataWrapperWorker::finished,
            worker, &ChatDataWrapperWorker::deleteLater);

    workerThread->start();
    QMetaObject::invokeMethod(worker,
                              &ChatDataWrapperWorker::start,
                              Qt::QueuedConnection);
}

int ChatDataWrapper::requestChatHistory() const
{
    int requestId = -1;
    if(!QMetaObject::invokeMethod(worker,
                                  &ChatDataWrapperWorker::requestChatHistory,
                                  Qt::BlockingQueuedConnection,
                                  &requestId)){
        qCritical() << "Chat history request failed";
    }

    return requestId;
}

int ChatDataWrapper::requestAddChatMessage(const NewChatMessageData &message)
{
    int requestId = -1;
    if(!QMetaObject::invokeMethod(worker,
                                  "requestAddChatMessage",
                                  Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(int, requestId),
                                  Q_ARG(NewChatMessageData, message))){
        qCritical() << "Add chat message request failed";
    }

    return requestId;
}
