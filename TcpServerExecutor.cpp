#include "TcpServerExecutor.h"

#include <QDataStream>
#include <QJsonDocument>
#include <QJsonObject>

#include "TcpDataTransmitter.h"

#include "MessageType.h"
#include "Result.h"
#include "MessageUtils.h"
#include "GetHistoryMessage.h"
#include "GetHistoryResponseMessage.h"
#include "AddMessageMessage.h"
#include "AddMessageResponseMessage.h"
#include "NotificationMessage.h"
#include "NewSessionRequestMessage.h"
#include "NewSessionResponseMessage.h"
#include "NewSessionConfirmMessage.h"
#include "NotificationType.h"
#include "ChatMessageData.h"
#include "utils.h"

#include "ChatDataWrapper.h"

const int SESSION_CONFIRM_TIMEOUT = 5000;

Result resultFromBool(bool result)
{
    if(result){
        return Result::Success;
    }
    else{
        return Result::Fail;
    }
}

TcpServerExecutor::TcpServerExecutor()
    : QObject(),
      chatDataWrapper(new ChatDataWrapper(this)),
      stopping(false)
{
    connect(chatDataWrapper, &ChatDataWrapper::chatHistoryRequestCompleted,
            this, &TcpServerExecutor::onChatHistoryRequestCompleted);
    connect(chatDataWrapper, &ChatDataWrapper::addChatMessageRequestCompleted,
            this, &TcpServerExecutor::onAddChatMessageRequestCompleted);
    connect(chatDataWrapper, &ChatDataWrapper::checkUsernameRequestCompleted,
            this, &TcpServerExecutor::onCheckUsernameCompleted);
}

TcpServerExecutor::~TcpServerExecutor()
{
    qDebug() << "~TcpServerExecutor()";
}

void TcpServerExecutor::start()
{

}

void TcpServerExecutor::stop()
{
    qDebug() << "TcpServerExecutor::stop()";

    if(clientSockets.empty()){
        emit finished();
        return;
    }    

    stopping = true;
    for(auto& it : clientSockets) {
        auto socket = it.second;
        if(socket->state() == QTcpSocket::ConnectedState){
            socket->disconnectFromHost();
        }
    }
}

void TcpServerExecutor::notifyAboutMessagesUpdate()
{
    qDebug() << "TcpServerExecutor::notifyAboutMessagesUpdate()";
    for(auto& item : userSessionsInfo){
        auto sessionId = item.first;
        auto socketId = item.second.socketId;

        if(!clientSockets.contains(socketId)){
            qCritical() << "No socket for this socket id: " << socketId;
            continue;
        }
        auto socket = clientSockets.at(socketId);

        auto sessionState = item.second.state;
        switch (sessionState) {
        case SessionState::Established:{
            auto message = std::make_shared<NotificationMessage>(sessionId, NotificationType::MessagesUpdated);
            TcpDataTransmitter::sendData(message->toJson().toJson(), *socket);
            qDebug() << "Messages updated notification sent";
            break;
        }
        case SessionState::GetHistoryRequest:
            sessionsToNotificate.emplace(sessionId);
            break;
        default:
            break;
        }
    }
}

void TcpServerExecutor::addClient(int socketDescriptor)
{
    auto tcpSocket = new QTcpSocket(this);
    if (!tcpSocket->setSocketDescriptor(socketDescriptor)){
        qDebug() << "setSocketDescriptor() failed";
        emit error(tcpSocket->error());
        return;
    }

    auto id = QUuid::createUuid();
    clientSockets.insert(std::make_pair(id, tcpSocket));

    connect(tcpSocket, &QTcpSocket::readyRead, this, [this, id](){
        onReadyReadMapped(id);
    });
    connect(tcpSocket, &QTcpSocket::errorOccurred, this, [this, tcpSocket](auto error){
        qWarning() << "Socket error occured: " << error;
        qWarning() << "Socket error description: " << tcpSocket->errorString();
    });
    connect(tcpSocket, &QTcpSocket::disconnected, this, [this, id](){
        onDisconnectedMapped(id);
    });
}

void TcpServerExecutor::onReadyReadMapped(const QUuid &id)
{
    qDebug() << "TcpServerExecutor::onReadyReadMapped()";
    auto socket = clientSockets[id];

    auto data = TcpDataTransmitter::receiveData(*socket);
    if(data.empty()){
        qWarning() << "No data received";
        return;
    }

    if(data.size() > 1){
        qWarning() << "More than one data array received";
        qWarning() << "Received data: " << data;
        //TDOD: Send error message to client
        return;
    }

    QJsonParseError jsonParseError;
    auto requestDoc = QJsonDocument::fromJson(data.at(0), &jsonParseError);
    if(requestDoc.isNull()){
        qDebug() << "Response parse error: " << jsonParseError.errorString();
        return;
    }

    auto message = MessageUtils::createMessageFromJson(requestDoc);
    auto messageType = message->getMessageType();
    switch (messageType) {
        case MessageType::NewSessionRequest:{
            auto sessionRequestMessage = std::dynamic_pointer_cast<NewSessionRequestMessage>(message);
            auto userId = sessionRequestMessage->getUserId();

            auto sessionId = createNewSession(id);
            userIdForSessionId[sessionId] = userId;
            auto requestId = chatDataWrapper->requestCheckUsername(sessionRequestMessage->getUsername());
            sessionForRequestInProcess[requestId] = sessionId;

            userSessionsInfo[sessionId].state = SessionState::EstablishingCheckingUsername;
            break;
        }
        case MessageType::NewSessionConfirm:{
            auto sessionRequestMessage = std::dynamic_pointer_cast<NewSessionConfirmMessage>(message);
            auto userId = sessionRequestMessage->getUserId();
            auto confirmSessionId = sessionRequestMessage->getSessionId();

            if(!userSessionsInfo.contains(confirmSessionId)){
                qWarning() << "No session with id: " << confirmSessionId;
                return;
            }
            if(userSessionsInfo[confirmSessionId].state != SessionState::EstablishingWaitingUserConfirm){
                qWarning() << "Session is not establishing!";
                return;
            }

            if(!userIdForSessionId.contains(confirmSessionId)){
                qWarning() << "No user id for session id: " << confirmSessionId;
                return;
            }
            auto initialUserId = userIdForSessionId[confirmSessionId];
            userIdForSessionId.erase(confirmSessionId);
            if(initialUserId != userId){
                qInfo() << "Invalid session id user id pair!";
                return;
            }

            userSessionsInfo[confirmSessionId].state = SessionState::Established;
            break;
        }
        case MessageType::GetHistory:{
            auto requestMessage = std::dynamic_pointer_cast<GetHistoryMessage>(message);
            auto sessionId = requestMessage->getSessionId();
            if(!userSessionsInfo.contains(sessionId)){
                qWarning() << "No session with id: " << sessionId;
                return;
            }

            if(!isSessionEstablished(sessionId)){
                qWarning() << "Session is not ready for new user request!";
                return;
            }

            auto requestId = chatDataWrapper->requestChatHistory();
            sessionForRequestInProcess[requestId] = sessionId;
            userSessionsInfo[sessionId].state = SessionState::GetHistoryRequest;
            break;
        }
        case MessageType::AddMessage:{
            auto sendMessage = std::dynamic_pointer_cast<AddMessageMessage>(message);
            auto sessionId = sendMessage->getSessionId();
            if(!userSessionsInfo.contains(sessionId)){
                qWarning() << "No session with id: " << sessionId;
                return;
            }

            if(!isSessionEstablished(sessionId)){
                qWarning() << "Session is not ready for new user request!";
                return;
            }

            auto requestId = chatDataWrapper->requestAddChatMessage(sendMessage->getChatMessageData());
            sessionForRequestInProcess[requestId] = sessionId;
            userSessionsInfo[sessionId].state = SessionState::AddMessageRequest;
            break;
        }
        default:
            qWarning() << "Received message has invalid type: " << messageTypeToString(messageType);
            break;
    }
}

void TcpServerExecutor::onDisconnectedMapped(const QUuid &id)
{
    clientSockets.erase(id);
    clientNotificationLists.erase(id);

    for(auto it = userSessionsInfo.begin(); it != userSessionsInfo.end();){
        if(it->second.socketId == id){
            userIdForSessionId.erase(it->first);
            it = userSessionsInfo.erase(it);
        }
        else{
            ++it;
        }
    }

    if(stopping && clientSockets.empty()){
        emit finished();
    }
}

void TcpServerExecutor::onChatHistoryRequestCompleted(int requestId, const std::vector<ChatMessageData> &history)
{
    assert(sessionForRequestInProcess.contains(requestId));

    auto sessionId = sessionForRequestInProcess[requestId];
    sessionForRequestInProcess.erase(requestId);
    if(!userSessionsInfo.contains(sessionId)){
        qDebug() << "No user session info for this session id: " << sessionId;
        return;
    }

    auto socketId = userSessionsInfo[sessionId].socketId;
    assert(clientSockets.contains(socketId));

    auto socket = clientSockets[socketId];
    GetHistoryResponseMessage responseMessage(sessionId, history);
    TcpDataTransmitter::sendData(responseMessage.toJson().toJson(), *socket);
    userSessionsInfo[sessionId].state = SessionState::Established;

    sendPendingNotification(sessionId);
}

void TcpServerExecutor::onAddChatMessageRequestCompleted(int requestId, bool result)
{
    assert(sessionForRequestInProcess.contains(requestId));

    auto sessionId = sessionForRequestInProcess[requestId];
    sessionForRequestInProcess.erase(requestId);
    if(!userSessionsInfo.contains(sessionId)){
        qDebug() << "No user session info for this session id: " << sessionId;
        return;
    }

    auto socketId = userSessionsInfo[sessionId].socketId;    
    assert(clientSockets.contains(socketId));

    auto socket = clientSockets[socketId];
    AddMessageResponseMessage responseMessage(sessionId, resultFromBool(result));
    TcpDataTransmitter::sendData(responseMessage.toJson().toJson(), *socket);
    userSessionsInfo[sessionId].state = SessionState::Established;

    notifyAboutMessagesUpdate();
}

void TcpServerExecutor::onCheckUsernameCompleted(int requestId, bool isValid)
{
    if(!sessionForRequestInProcess.contains(requestId)){
        //May be valid way becouse user can try to reconnect
        qInfo() << "Request id is not valid";
        return;
    }

    auto sessionId = sessionForRequestInProcess[requestId];
    sessionForRequestInProcess.erase(requestId);
    if(!userSessionsInfo.contains(sessionId)){
        qDebug() << "No user session info for this session id: " << sessionId;
        return;
    }

    if(userSessionsInfo[sessionId].state != SessionState::EstablishingCheckingUsername){
        qCritical() << "Wrong session sate";
        return;
    }

    auto socketId = userSessionsInfo[sessionId].socketId;
    assert(clientSockets.contains(socketId));

    auto socket = clientSockets[socketId];
    QUuid returnedSessionId;
    if(isValid){
        returnedSessionId = sessionId;
    }
    else{
        removeSession(sessionId);
    }

    if(!userIdForSessionId.contains(sessionId)){
        qCritical() << "No user id for session id: " << sessionId;
        return;
    }

    userSessionsInfo[sessionId].state = SessionState::EstablishingWaitingUserConfirm;

    NewSessionResponseMessage responseMessage(isValid,
                                              userIdForSessionId[sessionId],
                                              returnedSessionId);
    TcpDataTransmitter::sendData(responseMessage.toJson().toJson(), *socket);
}

void TcpServerExecutor::sendPendingNotification(const QUuid &sessionId)
{
    if(sessionsToNotificate.contains(sessionId)){
        if(!userSessionsInfo.contains(sessionId)){
            qCritical() << "No session info for this session id: " << sessionId;
        }
        auto sessionSocketId = userSessionsInfo[sessionId].socketId;

        if(!clientSockets.contains(sessionSocketId)){
            qCritical() << "No socket for this socket id: " << sessionSocketId;
        }
        auto socket = clientSockets[sessionSocketId];

        auto message = std::make_shared<NotificationMessage>(sessionId, NotificationType::MessagesUpdated);
        TcpDataTransmitter::sendData(message->toJson().toJson(), *socket);
        sessionsToNotificate.erase(sessionId);
    }
}

QUuid TcpServerExecutor::createNewSession(const QUuid &socketId)
{
    auto sessionId = QUuid::createUuid();
    userSessionsInfo[sessionId] = SessionInfo(socketId);
    return sessionId;
}

void TcpServerExecutor::removeSession(const QUuid &sessionId)
{
    if(!userSessionsInfo.contains(sessionId)){
        qWarning() << "No session with this id: " << sessionId;
    }

    userSessionsInfo.erase(sessionId);
}

QTcpSocket *TcpServerExecutor::getSessionSocket(const QUuid &sessionId)
{
    if(!userSessionsInfo.contains(sessionId)){
        qWarning() << "No user session info for this session id: " << sessionId;
        return nullptr;
    }

    auto socketId = userSessionsInfo[sessionId].socketId;
    if(!clientSockets.contains(socketId)){
        qWarning() << "No socket for this user socket id: " << socketId;
        return nullptr;
    }

    return clientSockets.at(socketId);
}

bool TcpServerExecutor::isSessionEstablished(const QUuid &sessionId)
{
    if(!userSessionsInfo.contains(sessionId)){
        qWarning() << "No user session info for this session id: " << sessionId;
        return false;
    }

    if(userSessionsInfo[sessionId].state != SessionState::Established){
        qWarning() << "Previous request is not finished!";
        //TODO: Process error
        return false;
    }

    return true;
}
