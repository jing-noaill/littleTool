#include "LicenseGeneratorDialog.h"
#include"qdebug.h"
LicenseGeneratorDialog::LicenseGeneratorDialog(QWidget* parent) : QDialog(parent)
{
	setupUI();
}

void LicenseGeneratorDialog::setupUI()
{
    QVBoxLayout* layout = new QVBoxLayout(this);

    // 机器码输入
    QLabel* machineLabel = new QLabel(u8"机器码:", this);
    machineCodeEdit = new QLineEdit(this);
    machineCodeEdit->setPlaceholderText(u8"粘贴机器指纹...");
   /* machineCodeEdit->setText(licenseSystem.generateMachineFingerprint());*/
    // 过期日期
    QLabel* expiryLabel = new QLabel(u8"过期日期:", this);
    QComboBox * comboBox = new QComboBox(this);
    comboBox->addItem(u8"1天");
    comboBox->addItem(u8"1周");
    comboBox->addItem(u8"1月");
    comboBox->addItem(u8"3月");
    comboBox->addItem(u8"6月");
    comboBox->addItem(u8"1年");
    comboBox->addItem(u8"3年");
    comboBox->addItem(u8"10年");

    expiryDateEdit = new QDateEdit(this);
    expiryDateEdit->setDate(QDate::currentDate().addDays(1));
    expiryDateEdit->setCalendarPopup(true);

    // 生成按钮
    QPushButton* generateBtn = new QPushButton(u8"生成注册码", this);
    connect(generateBtn, &QPushButton::clicked, this, &LicenseGeneratorDialog::generateLicense);

    // 注册码显示
    QLabel* licenseLabel = new QLabel(u8"生成的注册码:", this);
    licenseDisplay = new QPlainTextEdit(this);
    licenseDisplay->setReadOnly(true);

    // 布局
    layout->addWidget(machineLabel);
    layout->addWidget(machineCodeEdit);
    layout->addWidget(expiryLabel);
    layout->addWidget(comboBox);
    layout->addWidget(expiryDateEdit);
    layout->addWidget(generateBtn);
    layout->addWidget(licenseLabel);
    layout->addWidget(licenseDisplay);

    setLayout(layout);
    setWindowTitle(u8"注册码生成工具");
    resize(500, 400);
    connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
        [=](int index) {
            if (index == 0)
                expiryDateEdit->setDate(QDate::currentDate().addDays(1));
            else if (index == 1)
                expiryDateEdit->setDate(QDate::currentDate().addDays(7));
            else if (index == 2)
                expiryDateEdit->setDate(QDate::currentDate().addMonths(1));
            else if (index == 3)
                expiryDateEdit->setDate(QDate::currentDate().addMonths(3));
            else if (index == 4)
                expiryDateEdit->setDate(QDate::currentDate().addMonths(6));
            else if (index == 5)
                expiryDateEdit->setDate(QDate::currentDate().addYears(1));
            else if (index == 6)
                expiryDateEdit->setDate(QDate::currentDate().addYears(3));
            else if (index == 7)
                expiryDateEdit->setDate(QDate::currentDate().addYears(10));
        });

}

void LicenseGeneratorDialog::generateLicense()
{
    QString machineCode = machineCodeEdit->text().trimmed();
    
    qDebug() << "machineCode" << machineCode;
    QDate expiryDate = expiryDateEdit->date();

    if (machineCode.isEmpty()) {
        QMessageBox::warning(this, u8"错误",u8"请输入有效的机器码");
        return;
    }

    if (expiryDate <= QDate::currentDate()) {
        QMessageBox::warning(this, u8"错误", u8"过期日期必须大于当前日期");
        return;
    }

    QString license = licenseSystem.generateLicense(machineCode, expiryDate);
    qDebug() << "license:" << license;
   
    /* licenseSystem.validateLicense(license);*/
    licenseDisplay->setPlainText(license);

}
