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

#include "ChatDataProvider.h"

TcpServerExecutor::TcpServerExecutor(std::shared_ptr<ChatDataProvider> chatData)
    : QObject(),
      chatDataProvider(chatData),
      stopping(false)
{
}

TcpServerExecutor::~TcpServerExecutor()
{
    qDebug() << "~TcpServerExecutor()";
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

void TcpServerExecutor::onMessagesUpdated()
{
    qDebug() << "TcpServerExecutor::onMessagesUpdated()";
    auto message = std::make_shared<NotificationMessage>(NotificationType::MessagesUpdated);
    for(auto& item : awaitedExternalActions){
        auto clientId = item.first;
        if(item.second == OtherThreadAction::NoAction){
            auto socket = clientSockets.at(clientId);
            TcpDataTransmitter::sendData(message->toJson().toJson(), *socket);
            qDebug() << "Messages updated notification sent";
        }
        else{
            clientNotificationLists.at(clientId).push_back(message);
            qDebug() << "MessagesUpdated notification added to queue";
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
    awaitedExternalActions.insert(std::make_pair(id, OtherThreadAction::NoAction));
    clientNotificationLists.insert(std::make_pair(id, NotificationList()));
//    socketStates.push_back(std::make_pair(id, RequestSequence()));

    connect(tcpSocket, &QTcpSocket::readyRead, this, [this, id](){
        onReadyReadMapped(id);
    });
    connect(tcpSocket, &QTcpSocket::disconnected, this, [this, id](){
        onDisconnectedMapped(id);
    });
}

void TcpServerExecutor::onMessageAdded(bool success, const QUuid &clientId)
{
    if(awaitedExternalActions.at(clientId) != OtherThreadAction::AddMessage){
        qCritical() << "This excecutor instance is not waiting for message to be added!";
        return;
    }

    auto socket = clientSockets.at(clientId);
    SendMessageResponseMessage responseMessage;
    if(success){
        responseMessage.setResult(Result::Success);
    }
    else{
        responseMessage.setResult(Result::Fail);
    }
    TcpDataTransmitter::sendData(responseMessage.toJson().toJson(), *socket);
    qDebug() << "Response to add message sent";

    //Processing queued notifications
    auto clientNotificications = clientNotificationLists.at(clientId);
    if(!clientNotificications.empty()){
        for(auto& notification : clientNotificications){
            TcpDataTransmitter::sendData(notification->toJson().toJson(), *socket);
            qDebug() << "Queued notification sent";
        }
        clientNotificications.clear();
    }
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
            auto& chatHistory = chatDataProvider->getChatHistory();
            GetHistoryResponseMessage responseMessage(chatHistory);
            TcpDataTransmitter::sendData(responseMessage.toJson().toJson(), *socket);
            break;
        }
        case MessageType::SendMessage:{
            auto sentMessage = std::static_pointer_cast<SendMessageMessage>(message);
            auto messageObject = sentMessage->getMessageObject();
            emit newMessageReceived(messageObject, id);
            awaitedExternalActions.at(id) = OtherThreadAction::AddMessage;
            break;
        }
        default:
            qDebug() << "Received message have wrong type: " << messageTypeToString(messageType);
            break;
    }
}

void TcpServerExecutor::onDisconnectedMapped(const QUuid &id)
{
    qDebug() << "TcpServerExecutor::onDisconnectedMapped()";

    clientSockets[id]->deleteLater();
    clientSockets.erase(id);
    awaitedExternalActions.erase(id);
    clientNotificationLists.erase(id);

    if(stopping && clientSockets.empty()){
        emit finished();
    }
}
