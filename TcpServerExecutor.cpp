#include "TcpServerExecutor.h"

#include <QDataStream>
#include <QJsonDocument>
#include <QJsonObject>

#include "TcpDataTransmitter.h"
#include "ProtocolFormatSrings.h"

#include "ChatDataProvider.h"


const QString sendMessageValueString = ProtocolFormat::getProtocolStringLiteral(ProtocolFormat::ProtocolStringLiteral::SEND_CHAT_MESSAGE_REQUEST_TYPE);
const QString getHistoryValueString = ProtocolFormat::getProtocolStringLiteral(ProtocolFormat::ProtocolStringLiteral::GET_CHAT_HISTORY_REQUEST_TYPE);
const QString notificationValueString = ProtocolFormat::getProtocolStringLiteral(ProtocolFormat::ProtocolStringLiteral::NOTIFICATION_REQUEST_TYPE);
const QString typeParameterString = ProtocolFormat::getProtocolStringLiteral(ProtocolFormat::ProtocolStringLiteral::REQUEST_TYPE_KEY_NAME);
const QString messageParameterString = ProtocolFormat::getProtocolStringLiteral(ProtocolFormat::ProtocolStringLiteral::CHAT_MESSAGE_KEY);
const QString messagesParameterString = ProtocolFormat::getProtocolStringLiteral(ProtocolFormat::ProtocolStringLiteral::CHAT_HISTORY_KEY_NAME);
const QString requestResultParameterString = ProtocolFormat::getProtocolStringLiteral(ProtocolFormat::ProtocolStringLiteral::SEND_CHAT_MESSAGE_RESULT_KEY_NAME);
const QString completedValueString = ProtocolFormat::getProtocolStringLiteral(ProtocolFormat::ProtocolStringLiteral::SEND_CHAT_MESSAGE_RESULT_COMPLETED);
const QString failedValueString = ProtocolFormat::getProtocolStringLiteral(ProtocolFormat::ProtocolStringLiteral::SEND_CHAT_MESSAGE_RESULT_FAILED);
const QString notificationTypeParameterString = ProtocolFormat::getProtocolStringLiteral(ProtocolFormat::ProtocolStringLiteral::NOTIFICATION_TYPE_KEY_NAME);
const QString messagesUpdatedValueString = ProtocolFormat::getProtocolStringLiteral(ProtocolFormat::ProtocolStringLiteral::NOTIFICATION_MESSAGES_UPDATED);
const int WAIT_FOR_INCOMING_DATA_INTERVAL = 60 * 60 * 1000;


QJsonDocument createMessageDocument(const QString& typeValue, const QString& dataKey, const QJsonValue &dataValue){
    QJsonObject responseObject;
    responseObject.insert(typeParameterString, typeValue);
    responseObject.insert(dataKey, dataValue);

    QJsonDocument responseDocument;
    responseDocument.setObject(responseObject);

    return responseDocument;
}

QJsonDocument createSendMessageResponseDocument(const QString& status){
    return createMessageDocument(sendMessageValueString,
                                  requestResultParameterString,
                                  status);
}

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
    QJsonDocument notificationDocument = createMessageDocument(notificationValueString,
                                                                notificationTypeParameterString,
                                                                messagesUpdatedValueString);
    for(auto& it : clientSockets){
        TcpDataTransmitter::sendData(notificationDocument.toJson(), *it.second);
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
    socketStates.push_back(std::make_pair(id, RequestSequence()));

    connect(tcpSocket, &QTcpSocket::readyRead, this, [this, id](){
        onReadyReadMapped(id);
    });
    connect(tcpSocket, &QTcpSocket::disconnected, this, [this, id](){
        onDisconnectedMapped(id);
    });

//    connect(tcpSocket, &QTcpSocket::readyRead, readyReadSignalMapper, QOverload<>::of(&QSignalMapper::map));
//    readyReadSignalMapper->setMapping(tcpSocket, id.toString());
//    connect(readyReadSignalMapper, &QSignalMapper::mappedString, this, &TcpServerExecutor::onReadyReadMapped);
//    connect(tcpSocket, &QTcpSocket::disconnected, disconnectedSignalMapper, QOverload<>::of(&QSignalMapper::map));
//    disconnectedSignalMapper->setMapping(tcpSocket, id.toString());
//    connect(disconnectedSignalMapper, &QSignalMapper::mappedString, this, &TcpServerExecutor::onDisconnectedMapped);
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
    if(!requestDoc.isObject()){
        qDebug() << "Response is not JSON object";
        return;
    }

    auto requestObj = requestDoc.object();
    auto requestType = requestObj.value(typeParameterString).toString();
    qDebug() << "new request: " << requestType;
    if(requestType == getHistoryValueString){
        auto& chatHistory = chatDataProvider->getChatHistory();
        QJsonDocument responseDocument = createMessageDocument(getHistoryValueString,
                                                                messagesParameterString,
                                                                chatHistory);
        TcpDataTransmitter::sendData(responseDocument.toJson(), *socket);
    }
    else if(requestType == sendMessageValueString){
        auto message = requestObj.value(messageParameterString).toObject();
        if(chatDataProvider->addChatMessage(message)){
            auto responseDocument = createSendMessageResponseDocument(completedValueString);
            TcpDataTransmitter::sendData(responseDocument.toJson(), *socket);
        }
        else{
            auto responseDocument = createSendMessageResponseDocument(failedValueString);
            TcpDataTransmitter::sendData(responseDocument.toJson(), *socket);
            qDebug() << "Chat message add error";
        }
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
