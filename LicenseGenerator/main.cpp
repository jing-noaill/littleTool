#include <QtCore/QCoreApplication>
#include"LicenseGeneratorDialog.h"
#include <QTextCodec>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
  /*  QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));*/
    LicenseGeneratorDialog dlg;
    dlg.show();
    return a.exec();
   
}
