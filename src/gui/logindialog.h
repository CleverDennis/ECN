#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>

class RegisterDialog;  // 前向声明

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();

signals:
    void loginSuccess(const QString &token);

private slots:
    void onLoginClicked();
    void showRegisterDialog();
    void onLoginResponse(QNetworkReply *reply);
    void showError(const QString &message);

private:
    void setupUI();
    void setupConnections();

    QLineEdit *usernameEdit;
    QLineEdit *passwordEdit;
    QPushButton *loginButton;
    QPushButton *registerButton;
    QNetworkAccessManager *networkManager;
    RegisterDialog *registerDialog;
};

class RegisterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegisterDialog(QWidget *parent = nullptr);
    ~RegisterDialog();

signals:
    void registerSuccess();

private slots:
    void onRegisterClicked();
    void onRegisterResponse(QNetworkReply *reply);

private:
    void setupUI();
    void setupConnections();
    void showError(const QString &message);

    QLineEdit *usernameEdit;
    QLineEdit *passwordEdit;
    QLineEdit *confirmPasswordEdit;
    QPushButton *registerButton;
    QPushButton *cancelButton;
    QNetworkAccessManager *networkManager;
};

#endif // LOGINDIALOG_H 