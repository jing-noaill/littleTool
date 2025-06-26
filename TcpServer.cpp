#include "TcpServer.h"
#include<iostream>
#include <direct.h>
#include <io.h>

#include<filesystem>
#include<fstream>

TcpServer::TcpServer(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    //std::vector<std::string> data_csv;
    //for (int i = 0; i < 10; i++)
    //{
    //    data_csv.push_back(std::to_string(i));
    //}
    //writeToCsv("./a.csv", data_csv);
    init_tcp();
}

TcpServer::~TcpServer()
{}

void TcpServer::writeToCsv(std::string LocalFilePath, std::vector<std::string> data)
{
    bool flag_header = false;
    std::string dir = LocalFilePath.substr(0, LocalFilePath.rfind("/"));
    //判断该文件夹是否存在,文件夹不存在则创建
    if (_access(dir.c_str(), 0) == -1)
    {
        int flag = _mkdir(dir.c_str());  //创建文件夹
        if (flag == 0)//创建成功
            std::cout << "Create directory successfully." << std::endl;
        else //创建失败
            return;
    }
    //判断该文件是否存在,文件不存在则创表头
    if (_access(LocalFilePath.c_str(), 0) == -1)
        flag_header = true;
    std::ofstream file;
    //   所有输出附加在文件末尾,用追加的方式写入
    file.open(LocalFilePath, std::ios::app | std::ios::binary);
    if (!file.is_open())return;
    if (flag_header)
    {
        file << "ProgramName," << data.at(0) << "\n";
        file << "WorkOrder," << data.at(1) << "\n";
        file << "TestArea," << data.at(2) << "\n";
        file << "PcbSN," << data.at(3) << "\n";
        file << "TestTime," << data.at(4) << "\n";
        file << "CT," << data.at(5) << "\n";
        file << "PanelCount," << data.at(6) << "\n";
        file << "PanelCount_Pass," << data.at(7) << "\n";
        file << "PanelCount_Fail," << data.at(8) << "\n";
        file << "TOP," << data.at(9) << "\n";

        file << "产品SN,载具码,机种名,元件位号,元件类型,结果,设备报警类型,OP确认不良,OP复判时间,X偏移,Y偏移,角度偏移,直径,高度,偏心距,站位名,线别,OP工号,班次\n";
    }
    else
    {
        for (auto it : data)
        {
            file << it << ",";
        }
        file << std::endl;
    }
    file.close();

}

void TcpServer::init_str_cut()
{
   /* std::vector<QString>subsn;
    QString data = "{" \
        "\"data\": [" \
        "{" \
            "\"AssemblyCode\": \"N0102-000327*06BDC*HR*09015**35*240914*00876\","\
            "\"AssemblyNumber\" : 1,                                          "\
            "\"CheckTime\" : \"2024-09-17 13:40:45\",                           "\
            "\"Deviation\" : \"\",                                              "\
            "\"DeviceJudgment\" : \"NG\",                                       "\
            "\"Guid\" : 3069,                                                 "\
            "\"LineCode\" : \"35\",                                             "\
            "\"MachineType\" : 9,                                             "\
            "\"ManuFactor\" : \"安思\",                                         "\
            "\"PeopleJudgment\" : \"NG\",                                       "\
            "\"Poor\" : \"B缺件\",                                              "\
            "\"PoorDesc\" : \"LampBeads_LED_24\",                               "\
            "\"flag2\" : \" -12.531\",                                          "\
            "\"flag3\" : \" \"                                                  "\
        "}"\
    "]"\
    "}";
    QString data_out_str = data;
    int flag_jsonstr = data_out_str.indexOf("[");
    data_out_str = data_out_str.mid(flag_jsonstr);
    flag_jsonstr = data_out_str.indexOf("]") + 1;
    data_out_str = data_out_str.mid(0,flag_jsonstr);
    std::cout << "\n" << data_out_str.toStdString();*/
    QString data = "{\"EventID\":\"MES_Check\",\"Result\": \"OK\",\"MSG\": \"\",\"Need_Work\":\"PASS\",\"Return_Data\":{\"WorkOrder\":\"FC249L013A\",\"PanelSN\":\"YJ6030\",\"SUBSN\":\"\"}}";
    std::vector<QString> subsn;
    QString data_out_str = data;
    int flag_again = data.indexOf("SUBSN");
    if (flag_again != -1)
    {
        data_out_str = data_out_str.mid(flag_again + 1);
        flag_again = data_out_str.indexOf("\"", 5);
        data_out_str = data_out_str.mid(flag_again + 1);
        int flag_end = data_out_str.lastIndexOf("\"");
        data_out_str = data_out_str.mid(0, flag_end);
        if (data_out_str == "")return;
        while (1)
        {
            flag_again = data_out_str.indexOf(":");
            if (flag_again == -1)break;
            subsn.push_back(data_out_str.mid(0, flag_again));
            flag_again = data_out_str.indexOf(",");
            if (flag_again == -1)break;
            data_out_str = data_out_str.mid(flag_again + 1);
        }
    }

}

void TcpServer::init_tcp()
{
    tcpserver = nullptr;
    tcpsocket = nullptr;
    //创建监听套接字
    tcpserver = new QTcpServer(this);//指定父对象 回收空间

    //bind+listen
    tcpserver->listen(QHostAddress::Any, 8888);//绑定当前网卡所有的ip 绑定端口 也就是设置服务器地址和端口号

    //服务器建立连接
    connect(tcpserver, &QTcpServer::newConnection, [=]() {
        //取出连接好的套接字
        tcpsocket = tcpserver->nextPendingConnection();

        //获得通信套接字的控制信息
        QString ip = tcpsocket->peerAddress().toString();//获取连接的 ip地址
        quint16 port = tcpsocket->peerPort();//获取连接的 端口号
        QString temp = QString("[%1:%2] Connect Successful").arg(ip).arg(port);
        //显示连接成功
        ui.textEditRead->setText(temp);

        //接收信息  必须放到连接中的槽函数 不然tcpsocket就是一个野指针
        connect(tcpsocket, &QTcpSocket::readyRead, [=]() {
            //从通信套接字中取出内容
            QString str = tcpsocket->readAll();
            //在编辑区域显示
            ui.textEditRead->append("client:\t" + str);//不用settext 这样会覆盖之前的消息
            });
        });
}

void TcpServer::on_pushButtonsend_clicked()
{
    if (tcpsocket == nullptr) {
        return;
    }
    //获取编辑区域的内容
    QString str = ui.textEditWrite->toPlainText();

    //写入通信套接字 协议栈自动发送
    tcpsocket->write(str.toUtf8().data());

    //在编辑区域显示
    ui.textEditRead->append("Server:\t" + str);//不用settext 这样会覆盖之前的消息
}

void TcpServer::on_buttonclose_clicked()
{
    //通信套接字主动与服务端断开连接
    tcpsocket->disconnectFromHost();//结束聊天

    //关闭 通信套接字
    tcpsocket->close();

    tcpsocket = nullptr;
}

