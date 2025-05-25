#include "logindialog.h"
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QCryptographicHash>
#include "../../include/ecn_crypto.h"

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
    
    // 生成随机盐值（在实际应用中，应该从服务器获取存储的盐值）
    uint8_t salt[16];
    // TODO: 从服务器获取用户的盐值
    
    // 计算密码哈希
    QByteArray passwordHash = calculatePasswordHash(password, QByteArray((char*)salt, 16));
    
    // TODO: 发送登录请求到服务器
    // 临时模拟登录成功
    QByteArray sessionToken = QByteArray(64, 'A');  // 临时会话令牌
    emit loginSuccess(username, sessionToken);
    
    clearInputs();
}

void LoginDialog::onRegisterClicked()
{
    QString username = usernameEdit->text();
    QString password = passwordEdit->text();
    
    // 生成随机盐值
    uint8_t salt[16];
    if (!RAND_bytes(salt, sizeof(salt))) {
        QMessageBox::critical(this, tr("Error"),
                            tr("Failed to generate random salt."));
        return;
    }
    
    // 计算密码哈希
    QByteArray passwordHash = calculatePasswordHash(password, QByteArray((char*)salt, 16));
    
    // 生成SM2密钥对
    uint8_t publicKey[65];
    uint8_t privateKey[32];
    if (ecn_sm2_generate_keypair(publicKey, privateKey) != 0) {
        QMessageBox::critical(this, tr("Error"),
                            tr("Failed to generate SM2 key pair."));
        return;
    }
    
    // TODO: 发送注册请求到服务器
    // 临时显示成功消息
    QMessageBox::information(this, tr("Success"),
                           tr("Registration successful. Please login."));
    
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