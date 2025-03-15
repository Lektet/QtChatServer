#ifndef TCPSERVEREXECUTOR_H
#define TCPSERVEREXECUTOR_H

#include <QObject>
#include <QSignalMapper>
#include <QTcpSocket>
#include <QThread>
#include <QUuid>

#include <memory>
#include <map>
#include <list>
#include <unordered_set>

#include "UuidHash.h"

class ChatDataWrapper;
class SimpleMessage;
struct NewChatMessageData;
struct ChatMessageData;

class TcpServerExecutor : public QObject
{
    Q_OBJECT

public:
    TcpServerExecutor();
    ~TcpServerExecutor();

    void start();
    void stop();

    void notifyAboutMessagesUpdate();

public slots:
    void addClient(int socketDescriptor);

signals:
    void error(QTcpSocket::SocketError socketError);

    void finished();

private:    
    enum class ChatDataAction{
        NoAction,
        GetHistoryRequest,
        AddMessageRequest
    };

    ChatDataWrapper* chatDataWrapper;

    std::map<QUuid, QTcpSocket*> clientSockets;
    using NotificationList = std::list<std::shared_ptr<SimpleMessage>>;
    std::map<QUuid, NotificationList> clientNotificationLists;

    std::map<int, QUuid> socketIdWaitingForRequestCompleted;
    std::map<QUuid, ChatDataAction> socketCurrentAction;
    std::unordered_set<QUuid> socketIdToNotificate;

    bool stopping;

    void onStop();

    void onReadyReadMapped(const QUuid &id);
    void onDisconnectedMapped(const QUuid &id);

    void onChatHistoryRequestCompleted(int requestId, const std::vector<ChatMessageData>& history);
    void onAddChatMessageRequestCompleted(int requestId, bool result);

    void sendPendingNotification(const QUuid socketId, QTcpSocket* const socket);
};

#endif // TCPSERVEREXECUTOR_H
