#ifndef TCPSERVERTHREAD_H
#define TCPSERVERTHREAD_H

#include <QObject>
#include <QSignalMapper>
#include <QTcpSocket>
#include <QThread>
#include <QUuid>

#include <condition_variable>

enum class MessageType{
    SendChatMessage,
    GetChatHistory,
    Notification
};

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
//    QSignalMapper* readyReadSignalMapper;
//    QSignalMapper* disconnectedSignalMapper;

    std::map<QUuid, QTcpSocket*> clientSockets;
    using RequestSequence = std::list<MessageType>;
    std::list<std::pair<QUuid, RequestSequence>> socketStates;
    bool stopping;

    void onStop();

    void onReadyReadMapped(const QUuid &id);
    void onDisconnectedMapped(const QUuid &id);
};

#endif // TCPSERVERTHREAD_H
