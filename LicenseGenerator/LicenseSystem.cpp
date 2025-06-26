#include "LicenseSystem.h"
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QNetworkInterface>
#include <QStorageInfo>
#include <QSysInfo>
#include <QBuffer>
#include <QDataStream>
#include <QFile>
#include <QTimer>
#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

// 安全常量定义
const QByteArray LicenseSystem::MASTER_SALT = QByteArray::fromHex("3a7d5f2e8c1b4096");
const QString LicenseSystem::ENCRYPTION_KEY_PURPOSE = "LICENSE_ENCRYPTION_KEY_V4";
const QString LicenseSystem::SIGNATURE_KEY_PURPOSE = "LICENSE_SIGNATURE_KEY_V4";
const QString LicenseSystem::TIME_GUARD_FILE = "license_timeguard.dat";

LicenseSystem::LicenseSystem(QObject* parent) : QObject(parent), m_settings("YourCompany", "YourApp")
{
    initializeTimeGuard();

    // 设置定时检查时间完整性(每小时)
    QTimer* timeCheckTimer = new QTimer(this);
    connect(timeCheckTimer, &QTimer::timeout, this, [this]() {
        if (!checkTimeIntegrity()) {
            emit timeTamperingDetected(m_lastError);
        }
        });
    timeCheckTimer->start(3600000);
}

void LicenseSystem::setExpiryWarningDays(int daysBeforeWarning)
{
    m_warningDays = qMax(1, daysBeforeWarning);
}

// ========== 时间保护相关方法 ==========

void LicenseSystem::initializeTimeGuard()
{
    if (!QFile::exists(TIME_GUARD_FILE)) {
        TimeGuardData newData;
        newData.lastCheckTime = QDateTime::currentDateTime();
        newData.uptimeAtLastCheck = getSystemUptime();
        saveTimeGuardData(newData);
    }
}

bool LicenseSystem::checkTimeIntegrity()
{
    TimeGuardData stored = loadTimeGuardData();
    QDateTime current = QDateTime::currentDateTime();
    qint64 currentUptime = getSystemUptime();

    // 1. 检查时间是否回退
    if (current < stored.lastCheckTime) {
        m_lastError = "检测到系统时间被回退";
        return false;
    }

    // 2. 验证运行时间一致性
    qint64 elapsedTime = stored.lastCheckTime.msecsTo(current);
    qint64 expectedUptime = stored.uptimeAtLastCheck + elapsedTime;
    // 2. 检查是否正常重启
    if (currentUptime < stored.uptimeAtLastCheck) {
        // 更新存储时间为启动后时间
        TimeGuardData newData;
        newData.lastCheckTime = QDateTime::currentDateTime();
        newData.uptimeAtLastCheck = getSystemUptime();
        saveTimeGuardData(newData);
        return true;
    }
    // 允许2小时误差
    if (qAbs(currentUptime - expectedUptime) > 7200 * 1000) {
        m_lastError = QString("系统时间异常 (差异: %1秒)").arg((currentUptime - expectedUptime) / 1000);
        return false;
    }

    return true;
}

void LicenseSystem::updateTimeGuard()
{
    TimeGuardData newData;
    newData.lastCheckTime = QDateTime::currentDateTime();
    newData.uptimeAtLastCheck = getSystemUptime();
    saveTimeGuardData(newData);
}

LicenseSystem::TimeGuardData LicenseSystem::loadTimeGuardData() const
{
    TimeGuardData data;
    QFile file(TIME_GUARD_FILE);
    if (file.open(QIODevice::ReadOnly)) {
        QDataStream in(&file);
        in >> data.lastCheckTime >> data.uptimeAtLastCheck;
    }
    return data;
}

void LicenseSystem::saveTimeGuardData(const TimeGuardData& data)
{
    QFile file(TIME_GUARD_FILE);
    if (file.open(QIODevice::WriteOnly)) {
        QDataStream out(&file);
        out << data.lastCheckTime << data.uptimeAtLastCheck;
    }
}

qint64 LicenseSystem::getSystemUptime() const
{
#ifdef Q_OS_WIN
    return GetTickCount64();
#else
    QFile uptimeFile("/proc/uptime");
    if (uptimeFile.open(QIODevice::ReadOnly)) {
        QByteArray line = uptimeFile.readLine();
        QStringList parts = QString(line).split(' ');
        if (!parts.isEmpty()) {
            return parts[0].toDouble() * 1000; // 转为毫秒
        }
    }
    return -1;
#endif
}

// ========== 原有授权系统方法 ==========

QString LicenseSystem::generateMachineFingerprint() const
{
    QStringList components;
    components << QSysInfo::currentCpuArchitecture();
    components << QSysInfo::bootUniqueId();

    QList<QStorageInfo> drives = QStorageInfo::mountedVolumes();
    for (const QStorageInfo& drive : drives) {
        if (drive.isValid() && drive.isReady() && drive.isRoot()) {
            components << drive.device();
            break;
        }
    }

    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface& nic : interfaces) {  
        if (nic.type() == QNetworkInterface::Ethernet &&
            !(nic.flags() & QNetworkInterface::IsLoopBack)) {
            components << nic.hardwareAddress();
            break;
        }
    }

    QString fingerprint = components.join("|");
    QByteArray hash = QCryptographicHash::hash(fingerprint.toUtf8(), QCryptographicHash::Sha3_512);

    // 添加校验和
    quint16 checksum = qChecksum(hash.constData(), hash.size());
    return hash.toHex() + QString::number(checksum, 16).rightJustified(4, '0');
}

QString LicenseSystem::generateLicense(const QString& machineFingerprint, const QDate& expiryDate) const
{
    // 验证机器指纹
    if (machineFingerprint.length() != (128 + 4)) {
        qWarning() << "Invalid machine fingerprint format";
        return QString();
    }

    // 验证校验和
    QByteArray fingerprintData = QByteArray::fromHex(machineFingerprint.left(128).toLatin1());
    quint16 storedChecksum = machineFingerprint.right(4).toUShort(nullptr, 16);
    quint16 computedChecksum = qChecksum(fingerprintData.constData(), fingerprintData.size());

    if (storedChecksum != computedChecksum) {
        qWarning() << "Machine fingerprint checksum mismatch";
        return QString();
    }

    // 打包授权数据
    QByteArray licenseData = packLicenseData(machineFingerprint, expiryDate);
    if (licenseData.isEmpty()) {
        return QString();
    }

    // 生成签名
    QByteArray signature = generateSignature(licenseData);
    if (signature.isEmpty()) {
        return QString();
    }

    // 组合数据
    QByteArray licenseRaw = signature + licenseData;

    // Base64编码
    return licenseRaw.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
}

bool LicenseSystem::validateLicense(const QString& license)
{
    m_lastError.clear();

    // 1. 检查时间完整性
    if (!checkTimeIntegrity()) {
        return false;
    }

    // 2. Base64解码
    QByteArray licenseRaw = QByteArray::fromBase64(
        license.toUtf8(),
        QByteArray::Base64UrlEncoding |
        QByteArray::OmitTrailingEquals
    );

    if (licenseRaw.isEmpty()) {
        m_lastError = "注册码格式无效";
        return false;
    }

    // 3. 分离签名和数据
    if (licenseRaw.size() <= 64) {
        m_lastError = "注册码数据不完整";
        return false;
    }

    QByteArray signature = licenseRaw.left(64);
    QByteArray licenseData = licenseRaw.mid(64);

    // 4. 验证签名
    if (!verifySignature(licenseData, signature)) {
        m_lastError = "注册码签名验证失败";
        return false;
    }

    // 5. 解析数据
    QString machineFingerprint;
    QDate expiryDate;
    if (!unpackLicenseData(licenseData, machineFingerprint, expiryDate)) {
        m_lastError = "注册码数据解析失败";
        return false;
    }

    // 6. 验证机器指纹
    QString currentFingerprint = generateMachineFingerprint();
    if (!secureCompare(machineFingerprint.toUtf8(), currentFingerprint.toUtf8())) {
        m_lastError = "注册码与当前设备不匹配";
        return false;
    }

    // 7. 验证有效期
    int daysRemaining = QDate::currentDate().daysTo(expiryDate);
    if (daysRemaining < 0) {
        m_lastError = QString("注册码已过期 (有效期至 %1)").arg(expiryDate.toString("yyyy-MM-dd"));
        return false;
    }

    // 8. 检查是否需要发出到期警告
    if (daysRemaining <= m_warningDays) {
        emit licenseAboutToExpire(daysRemaining);
    }

    // 9. 更新时间保护
    updateTimeGuard();

    return true;
}

bool LicenseSystem::isLicensed() const
{
    if (!m_settings.contains("license_key")) {
        return false;
    }

    QString license = m_settings.value("license_key").toString();
    LicenseSystem validator;
    return validator.validateLicense(license);
}

QDate LicenseSystem::getExpiryDate() const
{
    if (!m_settings.contains("license_key")) {
        return QDate();
    }

    QString license = m_settings.value("license_key").toString();
    QByteArray licenseRaw = QByteArray::fromBase64(
        license.toUtf8(),
        QByteArray::Base64UrlEncoding |
        QByteArray::OmitTrailingEquals
    );

    if (licenseRaw.size() <= 64) {
        return QDate();
    }

    QByteArray licenseData = licenseRaw.mid(64);

    QString machineFingerprint;
    QDate expiryDate;
    LicenseSystem validator;
    if (validator.unpackLicenseData(licenseData, machineFingerprint, expiryDate)) {
        return expiryDate;
    }

    return QDate();
}

int LicenseSystem::getDaysRemaining() const
{
    QDate expiry = getExpiryDate();
    if (!expiry.isValid()) {
        return -1;
    }
    return QDate::currentDate().daysTo(expiry);
}

bool LicenseSystem::registerLicense(const QString& license)
{
    if (validateLicense(license)) {
        m_settings.setValue("license_key", license);
        m_settings.setValue("first_run_date", QDateTime::currentDateTime().toString("yyyyMMddhhmmss"));
        updateTimeGuard();
        return true;
    }
    return false;
}

// ========== 辅助方法 ==========

QByteArray LicenseSystem::generateSignature(const QByteArray& data) const
{
    QByteArray key = deriveKey(SIGNATURE_KEY_PURPOSE);
    if (key.isEmpty()) {
        return QByteArray();
    }

    return QMessageAuthenticationCode::hash(
        data,
        key,
        QCryptographicHash::Sha3_512
    );
}

bool LicenseSystem::verifySignature(const QByteArray& data, const QByteArray& signature) const
{
    QByteArray computedSig = generateSignature(data);
    return secureCompare(computedSig, signature);
}

QByteArray LicenseSystem::deriveKey(const QString& purpose) const
{
    QByteArray salt = QCryptographicHash::hash(
        MASTER_SALT + purpose.toUtf8(),
        QCryptographicHash::Sha3_512
    ).left(16);

    QByteArray input = purpose.toUtf8() + salt;
    QByteArray key;

    const int iterations = 100000;
    for (int i = 0; i < iterations; ++i) {
        input = QCryptographicHash::hash(input, QCryptographicHash::Sha3_512);
        if (key.isEmpty()) {
            key = input;
        }
        else {
            for (int j = 0; j < key.size() && j < input.size(); ++j) {
                key[j] = key.at(j) ^ input.at(j);
            }
        }
    }

    return key;
}

QByteArray LicenseSystem::packLicenseData(const QString& machineFingerprint, const QDate& expiryDate) const
{
    QByteArray data;
    QBuffer buffer(&data);
    if (!buffer.open(QIODevice::WriteOnly)) {
        return QByteArray();
    }

    QDataStream stream(&buffer);
    stream.setVersion(QDataStream::Qt_5_14);

    // 写入版本标记
    stream << quint8(0x04);

    // 写入机器指纹
    stream << machineFingerprint;

    // 写入过期日期
    stream << expiryDate.toString("yyyyMMdd");

    // 写入生成时间戳
    stream << QDateTime::currentDateTime().toString("yyyyMMddhhmmss");

    buffer.close();
    return data;
}

bool LicenseSystem::unpackLicenseData(const QByteArray& data, QString& outMachineFingerprint, QDate& outExpiryDate) const
{
    QBuffer buffer;
    buffer.setData(data);
    if (!buffer.open(QIODevice::ReadOnly)) {
        return false;
    }

    QDataStream stream(&buffer);
    stream.setVersion(QDataStream::Qt_5_14);

    // 读取版本标记
    quint8 version;
    stream >> version;
    if (version != 0x04) {
        buffer.close();
        return false;
    }

    // 读取机器指纹
    stream >> outMachineFingerprint;

    // 读取过期日期
    QString expiryStr;
    stream >> expiryStr;
    outExpiryDate = QDate::fromString(expiryStr, "yyyyMMdd");
    if (!outExpiryDate.isValid()) {
        buffer.close();
        return false;
    }

    // 读取生成时间戳
    QString generateTimeStr;
    stream >> generateTimeStr;

    buffer.close();
    return true;
}

bool LicenseSystem::secureCompare(const QByteArray& a, const QByteArray& b)
{
    if (a.size() != b.size()) {
        return false;
    }

    unsigned int result = 0;
    for (int i = 0; i < a.size(); ++i) {
        result |= static_cast<unsigned char>(a.at(i)) ^ static_cast<unsigned char>(b.at(i));
    }

    return result == 0;
}