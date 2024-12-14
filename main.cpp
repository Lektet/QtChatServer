#include "mainwindow.h"

#include <QCoreApplication>
#include <QLocale>
#include <QTranslator>

#include "TcpServer.h"

#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    auto tcpServer = new TcpServer(&a);
    if(!tcpServer->listen(QHostAddress::LocalHost, 44000)){
      qCritical() << "Unable to start server";
    }

    return a.exec();
}
