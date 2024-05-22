#ifndef TCPSERVERTHREAD_H
#define TCPSERVERTHREAD_H

#include <QObject>
#include <QTcpSocket>
#include <QThread>

#include <condition_variable>

class ChatDataProvider;

class TcpServerExecutor : public QObject
{
    Q_OBJECT

public:
    TcpServerExecutor(int socketDescriptor, std::shared_ptr<ChatDataProvider> chatData);

    void start();
    void stop();
    void wait();

    void onDataUpdate();

signals:
    void error(QTcpSocket::SocketError socketError);

    void executionFinished();

private:
    int socketDescriptor;
    std::shared_ptr<ChatDataProvider> chatDataProvider;
    std::shared_ptr<QTcpSocket> tcpSocket;
    QThread executorThread;

    void init();
    void onReadyRead();
    void onStop();
};

#endif // TCPSERVERTHREAD_H
