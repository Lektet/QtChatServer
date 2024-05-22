#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QTcpServer>

class ChatDataProvider;
class TcpServerExecutor;

class TcpServer : public QTcpServer
{
    Q_OBJECT


public:
    explicit TcpServer(std::shared_ptr<ChatDataProvider> chatData, QObject *parent = nullptr);
    ~TcpServer();

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    std::shared_ptr<ChatDataProvider> chatData;

//    std::vector<TcpServerExecutor*> serverExecutorList;

};

#endif // TCPSERVER_H
