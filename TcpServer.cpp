#include "TcpServer.h"

#include <QThread>
#include <QCoreApplication>

#include "TcpServerExecutor.h"
#include "ChatDataWrapper.h"
#include "NewChatMessageData.h"

#include <QDebug>

TcpServer::TcpServer(QObject *parent) :
    QTcpServer(parent),
    serverExecutor(new TcpServerExecutor()),
    executorThread(new QThread(this))
{
    connect(this, &TcpServer::acceptError, this, [this](auto error){
        qWarning() << "Accepting new connection error: " << error;
        qWarning() << "Accepting new connection error description: " << errorString();
    });

    serverExecutor->moveToThread(executorThread);

    connect(qApp, &QCoreApplication::aboutToQuit, serverExecutor, &TcpServerExecutor::stop, Qt::QueuedConnection);
    connect(serverExecutor, &TcpServerExecutor::finished, executorThread, &QThread::quit);
    connect(serverExecutor, &TcpServerExecutor::finished, serverExecutor, &TcpServerExecutor::deleteLater);
    executorThread->start();
}

TcpServer::~TcpServer()
{

}

void TcpServer::incomingConnection(qintptr socketDescriptor)
{
    QMetaObject::invokeMethod(serverExecutor,
                              "addClient",
                              Qt::QueuedConnection,
                              Q_ARG(int, socketDescriptor));
}
