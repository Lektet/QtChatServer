#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QHostAddress>

#include <QMessageBox>

#include "TcpServer.h"

#include "ChatDataProvider.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      tcpServer(new TcpServer(parent))
{
    ui->setupUi(this);

    if(!tcpServer->listen(QHostAddress::LocalHost, 44000)){
        QMessageBox::critical(this, tr("Unable to start server"), tr("Unable to start server"));
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

