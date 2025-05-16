#ifndef TCPSERVEREXECUTOR_H
#define TCPSERVEREXECUTOR_H

#include <QObject>
#include <QSignalMapper>
#include <QTcpSocket>
#include <QThread>
#include <QUuid>
#include <QTimer>

#include <memory>
#include <map>
#include <list>
#include <unordered_set>
#include <any>
#include <queue>

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
    enum class SessionState{
        Disconnected,
        Established,
        EstablishingCheckingUsername,
        EstablishingWaitingUserConfirm,
        GetHistoryRequest,
        AddMessageRequest
    };

    struct SessionInfo{
        SessionInfo(const QUuid& uuid = QUuid(),
                    SessionState sessionState = SessionState::Disconnected):
            socketId(uuid),
            state(sessionState)
        {
        }

        QUuid socketId;
        SessionState state;
    };

    ChatDataWrapper* chatDataWrapper;

    std::map<QUuid, QTcpSocket*> clientSockets;
    using NotificationList = std::list<std::shared_ptr<SimpleMessage>>;
    std::map<QUuid, NotificationList> clientNotificationLists;

    std::map<QUuid, QUuid> userIdForSessionId;
    // std::map<QUuid, QTimer*> sessionConfirmTimers;
    using UserSessionMap = std::map<int, QUuid>;
    UserSessionMap sessionForRequestInProcess;
    std::unordered_set<QUuid> sessionsToNotificate;

    std::map<QUuid, SessionInfo> userSessionsInfo;

    bool stopping;

    void onStop();

    void onReadyReadMapped(const QUuid &id);
    void onDisconnectedMapped(const QUuid &id);

    void onChatHistoryRequestCompleted(int requestId, const std::vector<ChatMessageData>& history);
    void onAddChatMessageRequestCompleted(int requestId, bool result);
    void onCheckUsernameCompleted(int requestId, bool isValid);

    void sendPendingNotification(const QUuid& sessionId);

    QUuid createNewSession(const QUuid &socketId);
    void removeSession(const QUuid& sessionId);

    QTcpSocket* getSessionSocket(const QUuid& sessionId);
    bool isSessionEstablished(const QUuid& sessionId);
};

#endif // TCPSERVEREXECUTOR_H
