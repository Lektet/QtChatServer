#include "TcpServer.h"

#include <QThread>
#include <QApplication>

#include "TcpServerExecutor.h"
#include "ChatDataProvider.h"

#include <QDebug>

TcpServer::TcpServer(std::shared_ptr<ChatDataProvider> chatData, QObject *parent) :
    QTcpServer(parent),
    chatData(chatData),
    serverExecutor(new TcpServerExecutor(chatData)),
    executorThread(new QThread(this))
{
    connect(chatData.get(), &ChatDataProvider::messagesUpdated, serverExecutor, &TcpServerExecutor::onDataUpdate, Qt::QueuedConnection);
    connect(qApp, &QApplication::aboutToQuit, serverExecutor, &TcpServerExecutor::stop, Qt::QueuedConnection);

    serverExecutor->moveToThread(executorThread);
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
