#ifndef SERVERWIDGET_H
#define SERVERWIDGET_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QtWidgets/QWidget>
#include "ui_TcpServer.h"

#include <cstdio>


class TcpServer : public QWidget
{
    Q_OBJECT

public:
    TcpServer(QWidget* parent = nullptr);
    ~TcpServer();
    void init_tcp();
    void init_str_cut();
    void writeToCsv(std::string LocalFilePath, std::vector<std::string> data);

private slots:
    void on_pushButtonsend_clicked();
    void on_buttonclose_clicked();


private:
    Ui::TcpServerClass ui;
    //声明两种套接字
    QTcpServer* tcpserver;
    QTcpSocket* tcpsocket;
};


#endif // SERVERWIDGET_H