#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QTcpServer>
#include <QThread>

class TcpServerExecutor;

class TcpServer : public QTcpServer
{
    Q_OBJECT


public:
    explicit TcpServer(QObject *parent = nullptr);
    ~TcpServer();

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    TcpServerExecutor* serverExecutor;
    QThread* executorThread;

//    std::vector<TcpServerExecutor*> serverExecutorList;
};

#endif // TCPSERVER_H
