#include <QtCore/QCoreApplication>
#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <direct.h>
#include <io.h>
#include<fstream>
#include <regex>

#include<QSqlDatabase>
#include <QSqlQuery>
#include <QVariant>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QFileInfo>
#include<QDateTime>

QSqlDatabase database;

void GetAllFiles(const std::string& folderPath, std::vector<std::string>& filePaths) {
    WIN32_FIND_DATAA findFileData;
    HANDLE hFind = INVALID_HANDLE_VALUE;

    std::string searchPath = folderPath + "\\*";
    hFind = FindFirstFileA(searchPath.c_str(), &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }

    do {
        const std::string fileOrDirName = findFileData.cFileName;

        if (fileOrDirName == "." || fileOrDirName == "..") {
            continue;
        }

        std::string fullPath = folderPath + "\\" + fileOrDirName;

        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // 如果是目录，递归调用
            GetAllFiles(fullPath, filePaths);
        }
        else {
            // 如果是文件，添加到列表
            if (fullPath.find("Alarm") != std::string::npos)
                filePaths.push_back(fullPath);
        }
    } while (FindNextFileA(hFind, &findFileData) != 0);

    FindClose(hFind);
}

void select(QString time, QString path)
{
    std::vector<int>board_vec;
    std::vector<QString>element_vec;
    QDateTime time_sql = QDateTime::fromString(time, "yyyyMMddhhmmss");
    auto time_again = time_sql.addSecs(-5).toString("yyyy-MM-dd HH:mm:ss");
    auto time_before = time_sql.addSecs(5).toString("yyyy-MM-dd HH:mm:ss");
    QSqlQuery query(QString("SELECT * "
        "FROM public.\"board_info\" "
        "WHERE create_time > '%1' and create_time < '%2';")
        .arg(time_again).arg(time_before),
        database);
    while (query.next())
    {
        board_vec.push_back(query.value("board_id").toInt());
    }
    QString name_item;
    {
        QString fileNameWithExt = QFileInfo(path).fileName();
        QString fileNameWithoutExt = fileNameWithExt.split('.').first();
        name_item = fileNameWithoutExt.split("Alarm-").last();
    }
    name_item = name_item + ".";
    for(auto it: board_vec)
    {
        QSqlQuery query_1(QString("SELECT * "
            "FROM public.\"element_info\" "
            "WHERE is_ok = false and board_id = %1;")
            .arg(it),
            database);
        while (query_1.next())
        {
            QString str = query_1.value("img_path").toString();
            QJsonDocument doc_ = QJsonDocument::fromJson(str.toUtf8());
            QJsonObject obj = doc_.object();
            QString total = obj.value("crop_source_image").toString(); 

            QString fileNameWithExt = QFileInfo(str).fileName();
            //QString fileNameWithoutExt = fileNameWithExt.split('.').first();
            //QString targetPart = fileNameWithoutExt.split("-").last();
            if (fileNameWithExt.contains(name_item))
            {
                QFile::rename(path, total);
                std::cout << "rename:" << std::endl 
                    << "old name:\t" << path.toStdString() 
                    << "new name:\t" << total.toStdString() << std::endl << std::endl;
                return;
            }
        }
    }
}

QString getFilePath()
{
    QString root_path = QCoreApplication::applicationDirPath();
    std::string path = root_path.toStdString();
    root_path.append("/setting.txt");

    return root_path;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    std::vector<std::string> Alarm_files_vec;
    QString folder = getFilePath(); 
    // 读取数据文件
    std::ifstream infile;
    // 将文件流对象与文件关联起来，默认以只读方式打开std::ios::in
    infile.open(folder.toStdString());
    if (!infile.is_open()) return -1;
    std::map<std::string, std::string> words;
    std::string work, data;
    while (std::getline(infile, work))
    {
        std::getline(infile, data);
        words[work] = data;
    }
    infile.close();

    if (database.isOpen())
    {
        database.close();
    }
    if (QSqlDatabase::contains("QPSQL"))
    {
        database = QSqlDatabase::database("QPSQL");
    }
    else
    {
        database = QSqlDatabase::addDatabase("QPSQL", "QPSQL");
    }
    database.setHostName(QString::fromStdString(words["host_ip"]));
    database.setPort(QString::fromStdString(words["port"]).toInt());
    database.setDatabaseName(QString::fromStdString(words["database_name"]));
    database.setUserName(QString::fromStdString(words["username"]));
    database.setPassword(QString::fromStdString(words["password"]));
    bool is_ok = database.open();

    std::string folderPath = QString::fromStdString(words["targetpath"]).toStdString();
    GetAllFiles(folderPath, Alarm_files_vec);
    for (auto path : Alarm_files_vec)
    {
        std::regex date_regex("\\\\([0-9]{8})\\\\");
        std::regex time_regex("\\\\([0-9]{6})\\\\");
        std::smatch matches;
        std::string time;
        if (std::regex_search(path, matches, date_regex)) {
            time = matches[1];
        }
        if (std::regex_search(path, matches, time_regex)) {
            time = time + std::string(matches[1]);
        }
        select(QString::fromStdString(time), QString::fromStdString(path));
    }
    database.close();
    std::cout << "it is end" << std::endl;
    app.exec();
    return 0;
    //return 0;
}


