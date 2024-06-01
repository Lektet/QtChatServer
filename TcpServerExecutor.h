#ifndef TCPSERVERTHREAD_H
#define TCPSERVERTHREAD_H

#include <QObject>
#include <QSignalMapper>
#include <QTcpSocket>
#include <QThread>
#include <QUuid>

class ChatDataProvider;

class TcpServerExecutor : public QObject
{
    Q_OBJECT

public:
    TcpServerExecutor(std::shared_ptr<ChatDataProvider> chatData);
    ~TcpServerExecutor();

    void stop();

    void onDataUpdate();

public slots:
    void addClient(int socketDescriptor);

signals:
    void error(QTcpSocket::SocketError socketError);

    void finished();

private:
    std::shared_ptr<ChatDataProvider> chatDataProvider;

    std::map<QUuid, QTcpSocket*> clientSockets;
    bool stopping;

    void onStop();

    void onReadyReadMapped(const QUuid &id);
    void onDisconnectedMapped(const QUuid &id);
};

#endif // TCPSERVERTHREAD_H
