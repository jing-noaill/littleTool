#pragma once
#include <QApplication>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QDateEdit>
#include <QMessageBox>
#include "LicenseSystem.h"
class LicenseGeneratorDialog : public QDialog {
    Q_OBJECT
public:
    LicenseGeneratorDialog(QWidget* parent = nullptr);

private slots:
    void generateLicense();
      
    

private:
    void setupUI();
     
    LicenseSystem licenseSystem;
    QLineEdit* machineCodeEdit;
    QDateEdit* expiryDateEdit;
    QPlainTextEdit* licenseDisplay;
};