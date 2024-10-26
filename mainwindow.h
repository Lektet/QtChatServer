#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class ChatDataProvider;
class TcpServer;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    TcpServer* tcpServer;
};
#endif // MAINWINDOW_H
