#include "mainwindow.h"

#include <QCoreApplication>
#include <QCommandLineParser>

#include <QUrl>

#include "TcpServer.h"

#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QCommandLineParser parser;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.setApplicationDescription("Qt chat server");
    parser.addHelpOption();

    QCommandLineOption hostOption("host", "Server host address", "host");
    parser.addOption(hostOption);
    QCommandLineOption portOption("port", "Server port number", "port");
    parser.addOption(portOption);

    parser.process(a);

    QHostAddress hostAddress(QHostAddress::LocalHost);
    if(parser.isSet(hostOption)){
        if(!hostAddress.setAddress(parser.value(hostOption))){
            qFatal() << "Invalid host address provided!";
            return 1;
        }
    }

    quint16 portNumber = 44000;
    if(parser.isSet(portOption)){
        bool convertResult = false;
        portNumber = parser.value(portOption).toUShort(&convertResult);
        if(!convertResult){
            qFatal() << "Invalid port number provided!";
            return 1;
        }
    }

    qInfo() << "Starting server with host: " << hostAddress
             << " and port: " << portNumber;
    auto tcpServer = new TcpServer(&a);
    if(!tcpServer->listen(hostAddress, portNumber)){
      qCritical() << "Unable to start server";
    }

    qInfo() << "Server started successfully";

    return a.exec();
}
