#include "TcpServer.h"

#include <QThread>
#include <QApplication>

#include "TcpServerExecutor.h"
#include "ChatDataProvider.h"

#include <QDebug>

TcpServer::TcpServer(std::shared_ptr<ChatDataProvider> chatProvider, QObject *parent) :
    QTcpServer(parent),
    chatDataProvider(chatProvider),
    serverExecutor(new TcpServerExecutor(chatDataProvider)),
    executorThread(new QThread(this))
{
    serverExecutor->moveToThread(executorThread);

    connect (serverExecutor, &TcpServerExecutor::newMessageReceived,
             this, [this](const QJsonObject& message, const QUuid& clientId){
        auto result = chatDataProvider->addChatMessage(message);
        QMetaObject::invokeMethod(serverExecutor,
                                  "onMessageAdded",
                                  Qt::QueuedConnection,
                                  Q_ARG(bool, result),
                                  Q_ARG(QUuid, clientId));
    }, Qt::QueuedConnection);
    connect(chatDataProvider.get(), &ChatDataProvider::messagesUpdated, serverExecutor, &TcpServerExecutor::onMessagesUpdated, Qt::QueuedConnection);
    connect(qApp, &QApplication::aboutToQuit, serverExecutor, &TcpServerExecutor::stop, Qt::QueuedConnection);

    connect(serverExecutor, &TcpServerExecutor::finished, executorThread, &QThread::quit);
    connect(executorThread, &QThread::finished, serverExecutor, &TcpServerExecutor::deleteLater);
    executorThread->start();
}

TcpServer::~TcpServer()
{
//    std::vector<std::pair<TcpServerExecutor*, std::future<void>>> futures;
//    for(auto ptr : serverExecutorList){
//        auto executorFuture = std::async(std::launch::async, [ptr](){
//            ptr->wait();
//        });
//        futures.push_back(std::make_pair(ptr, std::move(executorFuture)));
//    }

//    for(auto it = futures.begin(); it != futures.end(); it++){
//        it->second.get();
//        it->first->deleteLater();
//    }
}

void TcpServer::incomingConnection(qintptr socketDescriptor)
{
    QMetaObject::invokeMethod(serverExecutor,
                              "addClient",
                              Qt::QueuedConnection,
                              Q_ARG(int, socketDescriptor));
}
