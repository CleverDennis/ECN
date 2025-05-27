#include "logindialog.h"
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QCryptographicHash>
#include "../../include/ecn_crypto.h"
#include <QTcpSocket>
#include <QDataStream>
#include <QDebug>

// 服务器地址和端口
#define ECN_SERVER_HOST "localhost"
#define ECN_SERVER_PORT 8443

// 消息类型
#define ECN_MSG_REGISTER 1
#define ECN_MSG_LOGIN 2

// 注册请求结构体长度
#define ECN_REGISTER_REQ_LEN (32+64+65)
// 登录请求结构体长度
#define ECN_LOGIN_REQ_LEN (32+64)

LoginDialog::LoginDialog(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void LoginDialog::setupUi()
{
    // 创建布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // 创建标题标签
    QLabel *titleLabel = new QLabel(tr("ECN - Login"), this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);
    
    // 创建表单布局
    QGridLayout *formLayout = new QGridLayout();
    mainLayout->addLayout(formLayout);
    
    // 用户名输入
    formLayout->addWidget(new QLabel(tr("Username:"), this), 0, 0);
    usernameEdit = new QLineEdit(this);
    usernameEdit->setPlaceholderText(tr("Enter username"));
    formLayout->addWidget(usernameEdit, 0, 1);
    
    // 密码输入
    formLayout->addWidget(new QLabel(tr("Password:"), this), 1, 0);
    passwordEdit = new QLineEdit(this);
    passwordEdit->setPlaceholderText(tr("Enter password"));
    passwordEdit->setEchoMode(QLineEdit::Password);
    formLayout->addWidget(passwordEdit, 1, 1);
    
    // 按钮布局
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    mainLayout->addLayout(buttonLayout);
    
    // 登录按钮
    loginButton = new QPushButton(tr("Login"), this);
    loginButton->setEnabled(false);
    buttonLayout->addWidget(loginButton);
    
    // 注册按钮
    registerButton = new QPushButton(tr("Register"), this);
    registerButton->setEnabled(false);
    buttonLayout->addWidget(registerButton);
    
    // 状态标签
    statusLabel = new QLabel(this);
    statusLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(statusLabel);
    
    // 连接信号
    connect(usernameEdit, &QLineEdit::textChanged,
            this, &LoginDialog::onInputChanged);
    connect(passwordEdit, &QLineEdit::textChanged,
            this, &LoginDialog::onInputChanged);
    connect(loginButton, &QPushButton::clicked,
            this, &LoginDialog::onLoginClicked);
    connect(registerButton, &QPushButton::clicked,
            this, &LoginDialog::onRegisterClicked);
    
    // 设置布局间距
    mainLayout->setSpacing(20);
    formLayout->setSpacing(10);
    buttonLayout->setSpacing(10);
    
    // 设置边距
    mainLayout->setContentsMargins(40, 40, 40, 40);
}

void LoginDialog::onInputChanged()
{
    bool hasInput = !usernameEdit->text().isEmpty() && 
                   !passwordEdit->text().isEmpty();
    loginButton->setEnabled(hasInput);
    registerButton->setEnabled(hasInput);
    statusLabel->clear();
}

void LoginDialog::onLoginClicked()
{
    QString username = usernameEdit->text();
    QString password = passwordEdit->text();

    // 构造登录负载
    QByteArray payload;
    payload.append(username.toUtf8().leftJustified(32, '\0', true));
    payload.append(password.toUtf8().leftJustified(64, '\0', true));

    // 构造消息头（小端）
    QByteArray header;
    QDataStream ds(&header, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::LittleEndian);
    ds << (quint8)1; // version
    ds << (quint8)ECN_MSG_LOGIN; // type
    ds << (quint16)payload.size(); // payload_len
    header.append(QByteArray(64, '\0'));

    // 发送到服务器
    QTcpSocket sock;
    sock.connectToHost(ECN_SERVER_HOST, ECN_SERVER_PORT);
    if (!sock.waitForConnected(2000)) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to connect to server."));
        return;
    }
    sock.write(header + payload);
    sock.flush();
    if (!sock.waitForReadyRead(2000)) {
        QMessageBox::critical(this, tr("Error"), tr("No response from server."));
        sock.disconnectFromHost();
        return;
    }
    QByteArray respHeader = sock.read(68);
    if (respHeader.size() < 68) {
        QMessageBox::critical(this, tr("Error"), tr("Invalid response header."));
        sock.disconnectFromHost();
        return;
    }
    QDataStream respStream(respHeader);
    respStream.setByteOrder(QDataStream::LittleEndian);
    quint8 r_version, r_type;
    quint16 r_payload_len;
    respStream >> r_version >> r_type >> r_payload_len;
    respStream.skipRawData(64);
    QByteArray respPayload = sock.read(r_payload_len);
    if (respPayload.size() < r_payload_len) {
        QMessageBox::critical(this, tr("Error"), tr("Invalid response payload."));
        sock.disconnectFromHost();
        return;
    }
    quint8 error_code = (quint8)respPayload[0];
    if (error_code == 0) {
        // 登录成功，提取会话令牌
        QByteArray sessionToken = respPayload.mid(5, 64); // 5字节后是token
        emit loginSuccess(username, sessionToken);
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Login failed. Error code: %1").arg(error_code));
    }
    sock.disconnectFromHost();
    clearInputs();
}

void LoginDialog::onRegisterClicked()
{
    QString username = usernameEdit->text();
    QString password = passwordEdit->text();

    // 生成SM2密钥对
    uint8_t publicKey[65];
    uint8_t privateKey[32];
    if (ecn_sm2_generate_keypair(publicKey, privateKey) != 0) {
        QMessageBox::critical(this, tr("Error"),
                            tr("Failed to generate SM2 key pair."));
        return;
    }

    // 构造注册负载
    QByteArray payload;
    payload.append(username.toUtf8().leftJustified(32, '\0', true));
    payload.append(password.toUtf8().leftJustified(64, '\0', true));
    payload.append(QByteArray((const char*)publicKey, 65));

    // 构造消息头（小端）
    QByteArray header;
    QDataStream ds(&header, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::LittleEndian);
    ds << (quint8)1; // version
    ds << (quint8)ECN_MSG_REGISTER; // type
    ds << (quint16)payload.size(); // payload_len
    header.append(QByteArray(64, '\0'));

    // 发送到服务器
    QTcpSocket sock;
    sock.connectToHost(ECN_SERVER_HOST, ECN_SERVER_PORT);
    if (!sock.waitForConnected(2000)) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to connect to server."));
        return;
    }
    sock.write(header + payload);
    sock.flush();
    if (!sock.waitForReadyRead(2000)) {
        QMessageBox::critical(this, tr("Error"), tr("No response from server."));
        sock.disconnectFromHost();
        return;
    }
    QByteArray respHeader = sock.read(68);
    if (respHeader.size() < 68) {
        QMessageBox::critical(this, tr("Error"), tr("Invalid response header."));
        sock.disconnectFromHost();
        return;
    }
    QDataStream respStream(respHeader);
    respStream.setByteOrder(QDataStream::LittleEndian);
    quint8 r_version, r_type;
    quint16 r_payload_len;
    respStream >> r_version >> r_type >> r_payload_len;
    respStream.skipRawData(64);
    QByteArray respPayload = sock.read(r_payload_len);
    if (respPayload.size() < r_payload_len) {
        QMessageBox::critical(this, tr("Error"), tr("Invalid response payload."));
        sock.disconnectFromHost();
        return;
    }
    quint8 error_code = (quint8)respPayload[0];
    if (error_code == 0) {
        QMessageBox::information(this, tr("Success"), tr("Registration successful. Please login."));
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Registration failed. Error code: %1").arg(error_code));
    }
    sock.disconnectFromHost();
    clearInputs();
}

void LoginDialog::clearInputs()
{
    usernameEdit->clear();
    passwordEdit->clear();
    statusLabel->clear();
}

QByteArray LoginDialog::calculatePasswordHash(const QString &password, const QByteArray &salt)
{
    // 将密码和盐值组合
    QByteArray combined = password.toUtf8() + salt;
    
    // 使用SM3计算哈希
    uint8_t hash[32];
    if (ecn_sm3_hash((uint8_t*)combined.data(), combined.size(), hash) != 0) {
        QMessageBox::critical(this, tr("Error"),
                            tr("Failed to calculate password hash."));
        return QByteArray();
    }
    
    return QByteArray((char*)hash, 32);
} 