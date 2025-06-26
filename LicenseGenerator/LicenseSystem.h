#ifndef LICENSESYSTEM_H
#define LICENSESYSTEM_H

#include <QObject>
#include <QString>
#include <QDate>
#include <QByteArray>
#include <QSettings>
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>

class LicenseSystem : public QObject
{
    Q_OBJECT

public:
    explicit LicenseSystem(QObject* parent = nullptr);

    // 设置到期警告阈值(天)
    void setExpiryWarningDays(int daysBeforeWarning);

    // 生成机器指纹
    QString generateMachineFingerprint() const;

    // 生成注册码
    QString generateLicense(const QString& machineFingerprint, const QDate& expiryDate) const;

    // 验证注册码
    bool validateLicense(const QString& license);

    // 检查当前授权状态
    bool isLicensed() const;

    // 获取授权信息
    QDate getExpiryDate() const;
    int getDaysRemaining() const;
    QString getLicenseError() const;

    // 注册软件
    bool registerLicense(const QString& license);

signals:
    void licenseAboutToExpire(int daysRemaining);
    void timeTamperingDetected(const QString& details);

private:
    // 时间保护机制
    struct TimeGuardData {
        QDateTime lastCheckTime;
        qint64 uptimeAtLastCheck;
    };

    void initializeTimeGuard();
    bool checkTimeIntegrity();
    void updateTimeGuard();
    TimeGuardData loadTimeGuardData() const;
    void saveTimeGuardData(const TimeGuardData& data);
    qint64 getSystemUptime() const;

    // 加密/签名相关
    QByteArray generateSignature(const QByteArray& data) const;
    bool verifySignature(const QByteArray& data, const QByteArray& signature) const;
    QByteArray deriveKey(const QString& purpose) const;

    // 数据打包/解析
    QByteArray packLicenseData(const QString& machineFingerprint, const QDate& expiryDate) const;
    bool unpackLicenseData(const QByteArray& data, QString& outMachineFingerprint, QDate& outExpiryDate) const;

    // 安全比较
    static bool secureCompare(const QByteArray& a, const QByteArray& b);

    QString m_lastError;
    QSettings m_settings;
    int m_warningDays = 7; // 默认提前7天警告

    // 安全常量
    static const QByteArray MASTER_SALT;
    static const QString ENCRYPTION_KEY_PURPOSE;
    static const QString SIGNATURE_KEY_PURPOSE;
    static const QString TIME_GUARD_FILE;
};

#endif // LICENSESYSTEM_H