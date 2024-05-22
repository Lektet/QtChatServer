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

TcpServerExecutor::TcpServerExecutor(int socketDescriptor, std::shared_ptr<ChatDataProvider> chatData)
    : QObject(),
      socketDescriptor(socketDescriptor),
      chatDataProvider(chatData)
{
}

void TcpServerExecutor::start()
{
    this->moveToThread(&executorThread);
    connect(&executorThread, &QThread::started, this, &TcpServerExecutor::init);

    executorThread.start();
}

void TcpServerExecutor::init()
{
    tcpSocket = std::make_shared<QTcpSocket>();
    if (!tcpSocket->setSocketDescriptor(socketDescriptor)){
        qDebug() << "setSocketDescriptor() failed";
        emit error(tcpSocket->error());
        return;
    }

    connect(tcpSocket.get(), &QTcpSocket::readyRead, this, &TcpServerExecutor::onReadyRead, Qt::QueuedConnection);
    connect(tcpSocket.get(), &QTcpSocket::disconnected, this, &TcpServerExecutor::onStop);
    connect(&executorThread, &QThread::finished, this, &TcpServerExecutor::executionFinished);
}

void TcpServerExecutor::stop()
{
    if(tcpSocket->state() == QTcpSocket::ConnectedState){
        tcpSocket->disconnectFromHost();
    }
    else{
        QMetaObject::invokeMethod(this, &TcpServerExecutor::onStop, Qt::QueuedConnection);
    }
}

void TcpServerExecutor::wait()
{
    executorThread.wait();
}

void TcpServerExecutor::onDataUpdate()
{
    QJsonDocument notificationDocument = createMessageDocument(notificationValueString,
                                                                notificationTypeParameterString,
                                                                messagesUpdatedValueString);
    TcpDataTransmitter::sendData(notificationDocument.toJson(), *tcpSocket);
}

void TcpServerExecutor::onReadyRead()
{
    QByteArray data = TcpDataTransmitter::receiveData(*tcpSocket);

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
        TcpDataTransmitter::sendData(responseDocument.toJson(), *tcpSocket);
    }
    else if(requestType == sendMessageValueString){
        auto message = requestObj.value(messageParameterString).toObject();
        if(chatDataProvider->addChatMessage(message)){
            auto responseDocument = createSendMessageResponseDocument(completedValueString);
            TcpDataTransmitter::sendData(responseDocument.toJson(), *tcpSocket);
        }
        else{
            auto responseDocument = createSendMessageResponseDocument(failedValueString);
            TcpDataTransmitter::sendData(responseDocument.toJson(), *tcpSocket);
            qDebug() << "Chat message add error";
        }
    }
}

void TcpServerExecutor::onStop()
{
    QThread::currentThread()->quit();
}
