#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QWidget>
#include <QByteArray>

class QLineEdit;
class QPushButton;
class QLabel;

class LoginDialog : public QWidget {
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);

signals:
    void loginSuccess(const QString &username, const QByteArray &sessionToken);

private slots:
    void onLoginClicked();
    void onRegisterClicked();
    void onInputChanged();

private:
    void setupUi();
    void clearInputs();
    QByteArray calculatePasswordHash(const QString &password, const QByteArray &salt);

    QLineEdit *usernameEdit;
    QLineEdit *passwordEdit;
    QPushButton *loginButton;
    QPushButton *registerButton;
    QLabel *statusLabel;
};

#endif // LOGINDIALOG_H 