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

#include "ChatDataProvider.h"

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
      chatDataProvider(new ChatDataProvider(this)),
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

void TcpServerExecutor::notifyAboutMessagesUpdate()
{
    qDebug() << "TcpServerExecutor::onMessagesUpdated()";
    auto message = std::make_shared<NotificationMessage>(NotificationType::MessagesUpdated);
    for(auto& item : awaitedExternalActions){
        auto clientId = item.first;        
        auto socket = clientSockets.at(clientId);
        TcpDataTransmitter::sendData(message->toJson().toJson(), *socket);
        qDebug() << "Messages updated notification sent";
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
            auto chatHistory = chatDataProvider->getChatHistory();
            GetHistoryResponseMessage responseMessage(std::move(chatHistory));
            TcpDataTransmitter::sendData(responseMessage.toJson().toJson(), *socket);
            break;
        }
        case MessageType::SendMessage:{
            auto sentMessage = std::static_pointer_cast<SendMessageMessage>(message);
//            addMessage(sentMessage->getChatMessageData(), socket);
            auto success = chatDataProvider->addChatMessage(sentMessage->getChatMessageData());
            SendMessageResponseMessage responseMessage(resultFromBool(success));
            TcpDataTransmitter::sendData(responseMessage.toJson().toJson(), *socket);

            if(success){
                qDebug() << "New messages added successfully";
                notifyAboutMessagesUpdate();
            }
            else{
                qWarning() << "New message add failed";
            }
            break;
        }
        default:
            qDebug() << "Received message have a wrong type: " << messageTypeToString(messageType);
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
