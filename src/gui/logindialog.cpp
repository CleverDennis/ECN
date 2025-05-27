#include "logindialog.h"
#include <QMessageBox>
#include <QUrl>
#include <QUrlQuery>
#include <QFormLayout>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
    , networkManager(new QNetworkAccessManager(this))
    , registerDialog(nullptr)
{
    setupUI();
    setupConnections();
}

LoginDialog::~LoginDialog()
{
    if (registerDialog) {
        delete registerDialog;
    }
}

void LoginDialog::setupUI()
{
    setWindowTitle("登录");
    setFixedSize(300, 150);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 使用表单布局
    QFormLayout *formLayout = new QFormLayout;
    usernameEdit = new QLineEdit;
    passwordEdit = new QLineEdit;
    passwordEdit->setEchoMode(QLineEdit::Password);

    // 设置输入框固定宽度
    usernameEdit->setFixedWidth(200);
    passwordEdit->setFixedWidth(200);

    formLayout->addRow("用户名:", usernameEdit);
    formLayout->addRow("密码:", passwordEdit);

    // 按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    loginButton = new QPushButton("登录");
    registerButton = new QPushButton("注册");
    buttonLayout->addWidget(loginButton);
    buttonLayout->addWidget(registerButton);

    // 添加到主布局
    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(buttonLayout);
}

void LoginDialog::setupConnections()
{
    connect(loginButton, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(registerButton, &QPushButton::clicked, this, &LoginDialog::showRegisterDialog);
}

void LoginDialog::showRegisterDialog()
{
    if (!registerDialog) {
        registerDialog = new RegisterDialog(this);
        connect(registerDialog, &RegisterDialog::registerSuccess, [this]() {
            registerDialog->hide();
            usernameEdit->clear();
            passwordEdit->clear();
            usernameEdit->setFocus();
        });
    }
    registerDialog->show();
}

void LoginDialog::onLoginClicked()
{
    QString username = usernameEdit->text();
    QString password = passwordEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        showError("用户名和密码不能为空");
        return;
    }

    QJsonObject loginData;
    loginData["username"] = username;
    loginData["password"] = password;

    QNetworkRequest request(QUrl("http://localhost:8443/api/login"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = networkManager->post(request, QJsonDocument(loginData).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onLoginResponse(reply); });
}

void LoginDialog::onLoginResponse(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject response = doc.object();
        if (response.contains("token")) {
            emit loginSuccess(response["token"].toString());
            accept();
        } else {
            showError("登录失败：服务器响应格式错误");
        }
    } else {
        showError("登录失败：" + reply->errorString());
    }
    reply->deleteLater();
}

void LoginDialog::showError(const QString &message)
{
    QMessageBox::critical(this, "错误", message);
}

// RegisterDialog 实现
RegisterDialog::RegisterDialog(QWidget *parent)
    : QDialog(parent)
    , networkManager(new QNetworkAccessManager(this))
{
    setupUI();
    setupConnections();
}

RegisterDialog::~RegisterDialog()
{
}

void RegisterDialog::setupUI()
{
    setWindowTitle("注册新用户");
    setFixedSize(300, 200);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 使用表单布局
    QFormLayout *formLayout = new QFormLayout;
    usernameEdit = new QLineEdit;
    passwordEdit = new QLineEdit;
    confirmPasswordEdit = new QLineEdit;
    passwordEdit->setEchoMode(QLineEdit::Password);
    confirmPasswordEdit->setEchoMode(QLineEdit::Password);

    // 设置输入框固定宽度
    usernameEdit->setFixedWidth(200);
    passwordEdit->setFixedWidth(200);
    confirmPasswordEdit->setFixedWidth(200);

    formLayout->addRow("用户名:", usernameEdit);
    formLayout->addRow("密码:", passwordEdit);
    formLayout->addRow("确认密码:", confirmPasswordEdit);

    // 按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    registerButton = new QPushButton("注册");
    cancelButton = new QPushButton("取消");
    buttonLayout->addWidget(registerButton);
    buttonLayout->addWidget(cancelButton);

    // 添加到主布局
    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(buttonLayout);
}

void RegisterDialog::setupConnections()
{
    connect(registerButton, &QPushButton::clicked, this, &RegisterDialog::onRegisterClicked);
    connect(cancelButton, &QPushButton::clicked, this, &RegisterDialog::reject);
}

void RegisterDialog::onRegisterClicked()
{
    QString username = usernameEdit->text();
    QString password = passwordEdit->text();
    QString confirmPassword = confirmPasswordEdit->text();

    if (username.isEmpty() || password.isEmpty() || confirmPassword.isEmpty()) {
        showError("所有字段都必须填写");
        return;
    }

    if (password != confirmPassword) {
        showError("两次输入的密码不一致");
        return;
    }

    QJsonObject registerData;
    registerData["username"] = username;
    registerData["password"] = password;

    QNetworkRequest request(QUrl("http://localhost:8443/api/register"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = networkManager->post(request, QJsonDocument(registerData).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onRegisterResponse(reply); });
}

void RegisterDialog::onRegisterResponse(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QMessageBox::information(this, "注册成功", "请使用新账号登录");
        emit registerSuccess();
        accept();
    } else {
        showError("注册失败：" + reply->errorString());
    }
    reply->deleteLater();
}

void RegisterDialog::showError(const QString &message)
{
    QMessageBox::critical(this, "错误", message);
} 