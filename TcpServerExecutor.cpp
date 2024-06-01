#include "TcpServerExecutor.h"

#include <QDataStream>
#include <QJsonDocument>
#include <QJsonObject>

#include "TcpDataTransmitter.h"
#include "ProtocolFormatSrings.h"

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
//      readyReadSignalMapper(new QSignalMapper(this)),
//      disconnectedSignalMapper(new QSignalMapper(this)),
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

void TcpServerExecutor::onDataUpdate()
{
    NotificationMessage message(NotificationType::MessagesUpdated);
    for(auto& it : clientSockets){
        TcpDataTransmitter::sendData(message.toJson().toJson(), *it.second);
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
    auto socket = clientSockets[id];

    QByteArray data = TcpDataTransmitter::receiveData(*socket);

    QJsonParseError jsonParseError;
    auto requestDoc = QJsonDocument::fromJson(data, &jsonParseError);
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
            auto messageText = sentMessage->getMessageData();
            if(chatDataProvider->addChatMessage(messageText)){
                SendMessageResponseMessage responseMessage(Result::Success);
                TcpDataTransmitter::sendData(responseMessage.toJson().toJson(), *socket);
                break;
            }
            else{
                SendMessageResponseMessage responseMessage(Result::Fail);
                TcpDataTransmitter::sendData(responseMessage.toJson().toJson(), *socket);
                qDebug() << "Chat message add error";
                break;
            }
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

    if(stopping && clientSockets.empty()){
        emit finished();
    }
}
