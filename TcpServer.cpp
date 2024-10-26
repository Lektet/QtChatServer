#include "TcpServer.h"

#include <QThread>
#include <QApplication>

#include "TcpServerExecutor.h"
#include "ChatDataProvider.h"
#include "NewChatMessageData.h"

#include <QDebug>

TcpServer::TcpServer(QObject *parent) :
    QTcpServer(parent),
    serverExecutor(new TcpServerExecutor()),
    executorThread(new QThread(this))
{
    serverExecutor->moveToThread(executorThread);

    connect(qApp, &QApplication::aboutToQuit, serverExecutor, &TcpServerExecutor::stop, Qt::QueuedConnection);
    connect(serverExecutor, &TcpServerExecutor::finished, executorThread, &QThread::quit);
    connect(executorThread, &QThread::finished, serverExecutor, &TcpServerExecutor::deleteLater);
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
