#ifndef TCPSERVERTHREAD_H
#define TCPSERVERTHREAD_H

#include <QObject>
#include <QSignalMapper>
#include <QTcpSocket>
#include <QThread>
#include <QUuid>

#include <memory>
#include <map>
#include <list>

class ChatDataProvider;
class SimpleMessage;

class TcpServerExecutor : public QObject
{
    Q_OBJECT

public:
    TcpServerExecutor(std::shared_ptr<ChatDataProvider> chatData);
    ~TcpServerExecutor();

    void stop();

    void onMessagesUpdated();

public slots:
    void addClient(int socketDescriptor);

    void onMessageAdded(bool success, const QUuid& clientId);

signals:
    void newMessageReceived(const QJsonObject& message, const QUuid& clientId);

    void error(QTcpSocket::SocketError socketError);

    void finished();

private:    
    enum class OtherThreadAction{
        NoAction,
        AddMessage
    };

    std::shared_ptr<ChatDataProvider> chatDataProvider;

    std::map<QUuid, QTcpSocket*> clientSockets;
    std::map<QUuid, OtherThreadAction> awaitedExternalActions;
    using NotificationList = std::list<std::shared_ptr<SimpleMessage>>;
    std::map<QUuid, NotificationList> clientNotificationLists;

    bool stopping;

    void onStop();

    void onReadyReadMapped(const QUuid &id);
    void onDisconnectedMapped(const QUuid &id);
};

#endif // TCPSERVERTHREAD_H
