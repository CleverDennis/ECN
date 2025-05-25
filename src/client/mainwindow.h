#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QTcpSocket>
#include "../../include/ecn_protocol.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class LoginDialog;
class NoteListWidget;
class NoteEditWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onLoginSuccess(const QString &username, const QByteArray &sessionToken);
    void onLogout();
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError socketError);
    void onDataReceived();

private:
    void setupUi();
    void connectToServer();
    void disconnectFromServer();
    void sendMessage(uint8_t type, const QByteArray &payload);

    Ui::MainWindow *ui;
    QStackedWidget *stackedWidget;
    LoginDialog *loginDialog;
    NoteListWidget *noteListWidget;
    NoteEditWidget *noteEditWidget;
    QTcpSocket *socket;

    QString currentUsername;
    QByteArray sessionToken;
    QByteArray receiveBuffer;
};

#endif // MAINWINDOW_H 