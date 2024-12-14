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
#include "SendMessageMessage.h"
#include "SendMessageResponseMessage.h"
#include "NotificationMessage.h"
#include "NotificationType.h"
#include "ChatMessageData.h"
#include "utils.h"

#include "ChatDataWrapper.h"

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
    auto message = std::make_shared<NotificationMessage>(NotificationType::MessagesUpdated);
    for(auto& item : clientSockets){
        auto clientId = item.first;
        auto socket = item.second;

        auto currentAction = socketCurrentAction.at(clientId);
        switch (currentAction) {
        case ChatDataAction::NoAction:
            TcpDataTransmitter::sendData(message->toJson().toJson(), *socket);
            qDebug() << "Messages updated notification sent";
            break;
        case ChatDataAction::GetHistoryRequest:
            socketNeedingNotificationIds.emplace(clientId);
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
//    awaitedExternalActions.insert(std::make_pair(id, OtherThreadAction::NoAction));
    socketCurrentAction[id] = ChatDataAction::NoAction;
    clientNotificationLists.insert(std::make_pair(id, NotificationList()));
//    socketStates.push_back(std::make_pair(id, RequestSequence()));

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
        case MessageType::GetHistory:{
            auto requestId = chatDataWrapper->requestChatHistory();
            socketWaitingForResultIds[requestId] = id;
            socketCurrentAction[id] = ChatDataAction::GetHistoryRequest;
            break;
        }
        case MessageType::SendMessage:{
            auto sendMessage = std::static_pointer_cast<SendMessageMessage>(message);
            auto requestId = chatDataWrapper->requestAddChatMessage(sendMessage->getChatMessageData());
            socketWaitingForResultIds[requestId] = id;
            socketCurrentAction[id] = ChatDataAction::AddMessageRequest;
            break;
        }
        default:
            qDebug() << "Received message has invalid type: " << messageTypeToString(messageType);
            break;
    }
}

void TcpServerExecutor::onDisconnectedMapped(const QUuid &id)
{
    qDebug() << "TcpServerExecutor::onDisconnectedMapped()";

    clientSockets[id]->deleteLater();
    clientSockets.erase(id);
    socketCurrentAction.erase(id);
    clientNotificationLists.erase(id);

    if(stopping && clientSockets.empty()){
        emit finished();
    }
}

void TcpServerExecutor::onChatHistoryRequestCompleted(int requestId, const std::vector<ChatMessageData> &history)
{
    if(!socketWaitingForResultIds.contains(requestId)){
        qCritical() << "Invalid completed request id!";
        return;
    }

    auto socketId = socketWaitingForResultIds.at(requestId);
    if(!clientSockets.contains(socketId)){
        qCritical() << "Invalid socket id!";
        return;
    }

    auto socket = clientSockets.at(socketId);
    GetHistoryResponseMessage responseMessage(history);
    TcpDataTransmitter::sendData(responseMessage.toJson().toJson(), *socket);
    socketCurrentAction[socketId] = ChatDataAction::NoAction;

    sendPendingNotification(socketId, socket);
}

void TcpServerExecutor::onAddChatMessageRequestCompleted(int requestId, bool result)
{
    if(!socketWaitingForResultIds.contains(requestId)){
        qCritical() << "Invalid completed request id!";
        return;
    }

    auto socketId = socketWaitingForResultIds.at(requestId);
    if(!clientSockets.contains(socketId)){
        qCritical() << "Invalid socket id!";
        return;
    }

    auto socket = clientSockets.at(socketId);
    SendMessageResponseMessage responseMessage(resultFromBool(result));
    TcpDataTransmitter::sendData(responseMessage.toJson().toJson(), *socket);
    socketCurrentAction[socketId] = ChatDataAction::NoAction;

    notifyAboutMessagesUpdate();
}

void TcpServerExecutor::sendPendingNotification(const QUuid socketId, QTcpSocket * const socket)
{
    assert(socket != nullptr);

    if(socketNeedingNotificationIds.contains(socketId)){
        auto message = std::make_shared<NotificationMessage>(NotificationType::MessagesUpdated);
        TcpDataTransmitter::sendData(message->toJson().toJson(), *socket);
        socketNeedingNotificationIds.erase(socketId);
    }
}
