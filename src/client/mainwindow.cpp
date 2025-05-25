#include "mainwindow.h"
#include "logindialog.h"
#include "notelistwidget.h"
#include "noteeditwidget.h"
#include <QMessageBox>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QMenu>
#include <QAction>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , socket(new QTcpSocket(this))
{
    setupUi();
    
    // 连接socket信号
    connect(socket, &QTcpSocket::connected, this, &MainWindow::onConnected);
    connect(socket, &QTcpSocket::disconnected, this, &MainWindow::onDisconnected);
    connect(socket, &QTcpSocket::errorOccurred, this, &MainWindow::onError);
    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::onDataReceived);
    
    // 连接到服务器
    connectToServer();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupUi()
{
    // 设置窗口标题和大小
    setWindowTitle(tr("ECN - Encrypted Cloud Notes"));
    resize(800, 600);
    
    // 创建中心部件
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // 创建布局
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    
    // 创建堆叠部件
    stackedWidget = new QStackedWidget(this);
    layout->addWidget(stackedWidget);
    
    // 创建登录对话框
    loginDialog = new LoginDialog(this);
    connect(loginDialog, &LoginDialog::loginSuccess,
            this, &MainWindow::onLoginSuccess);
    
    // 创建笔记列表部件
    noteListWidget = new NoteListWidget(this);
    
    // 创建笔记编辑部件
    noteEditWidget = new NoteEditWidget(this);
    
    // 添加部件到堆叠部件
    stackedWidget->addWidget(loginDialog);
    stackedWidget->addWidget(noteListWidget);
    stackedWidget->addWidget(noteEditWidget);
    
    // 创建菜单栏
    QMenuBar *menuBar = new QMenuBar(this);
    setMenuBar(menuBar);
    
    // 文件菜单
    QMenu *fileMenu = menuBar->addMenu(tr("File"));
    
    // 注销动作
    QAction *logoutAction = new QAction(tr("Logout"), this);
    connect(logoutAction, &QAction::triggered, this, &MainWindow::onLogout);
    fileMenu->addAction(logoutAction);
    
    // 退出动作
    QAction *exitAction = new QAction(tr("Exit"), this);
    connect(exitAction, &QAction::triggered, this, &QApplication::quit);
    fileMenu->addAction(exitAction);
    
    // 显示登录对话框
    stackedWidget->setCurrentWidget(loginDialog);
}

void MainWindow::connectToServer()
{
    socket->connectToHost("localhost", 8443);
}

void MainWindow::disconnectFromServer()
{
    if (socket->state() != QAbstractSocket::UnconnectedState) {
        socket->disconnectFromHost();
    }
}

void MainWindow::sendMessage(uint8_t type, const QByteArray &payload)
{
    // 构造消息头
    ecn_msg_header_t header;
    header.version = ECN_PROTOCOL_VERSION;
    header.type = type;
    header.payload_len = payload.size();
    memcpy(header.session_token, sessionToken.data(),
           qMin(sessionToken.size(), (int)sizeof(header.session_token)));
    
    // 发送消息
    QByteArray message;
    message.append((const char *)&header, sizeof(header));
    message.append(payload);
    socket->write(message);
}

void MainWindow::onLoginSuccess(const QString &username, const QByteArray &token)
{
    currentUsername = username;
    sessionToken = token;
    
    // 切换到笔记列表界面
    stackedWidget->setCurrentWidget(noteListWidget);
    
    // 更新窗口标题
    setWindowTitle(tr("ECN - %1").arg(username));
    
    // 请求笔记列表
    sendMessage(ECN_MSG_NOTE_LIST, QByteArray());
}

void MainWindow::onLogout()
{
    // 发送登出请求
    sendMessage(ECN_MSG_LOGOUT, QByteArray());
    
    // 清除会话信息
    currentUsername.clear();
    sessionToken.clear();
    
    // 切换到登录界面
    stackedWidget->setCurrentWidget(loginDialog);
    setWindowTitle(tr("ECN - Encrypted Cloud Notes"));
}

void MainWindow::onConnected()
{
    qDebug() << "Connected to server";
}

void MainWindow::onDisconnected()
{
    qDebug() << "Disconnected from server";
    
    // 如果不是主动登出，显示错误消息
    if (!currentUsername.isEmpty()) {
        QMessageBox::warning(this, tr("Connection Lost"),
                           tr("Lost connection to server. Please try again."));
        onLogout();
    }
}

void MainWindow::onError(QAbstractSocket::SocketError socketError)
{
    QString errorMessage;
    switch (socketError) {
        case QAbstractSocket::ConnectionRefusedError:
            errorMessage = tr("The server refused the connection.");
            break;
        case QAbstractSocket::RemoteHostClosedError:
            errorMessage = tr("The server closed the connection.");
            break;
        case QAbstractSocket::HostNotFoundError:
            errorMessage = tr("The server was not found.");
            break;
        case QAbstractSocket::SocketTimeoutError:
            errorMessage = tr("The connection timed out.");
            break;
        default:
            errorMessage = tr("An error occurred: %1.")
                          .arg(socket->errorString());
    }
    
    QMessageBox::critical(this, tr("Network Error"), errorMessage);
}

void MainWindow::onDataReceived()
{
    // 读取所有可用数据
    receiveBuffer.append(socket->readAll());
    
    // 处理完整的消息
    while (receiveBuffer.size() >= (int)sizeof(ecn_msg_header_t)) {
        // 解析消息头
        const ecn_msg_header_t *header = 
            reinterpret_cast<const ecn_msg_header_t *>(receiveBuffer.constData());
        
        // 检查消息完整性
        int totalSize = sizeof(ecn_msg_header_t) + header->payload_len;
        if (receiveBuffer.size() < totalSize) {
            break;  // 等待更多数据
        }
        
        // 提取负载
        QByteArray payload = receiveBuffer.mid(sizeof(ecn_msg_header_t),
                                             header->payload_len);
        
        // 处理消息
        switch (header->type) {
            case ECN_MSG_RESPONSE:
                handleResponse(payload);
                break;
            case ECN_MSG_ERROR:
                handleError(payload);
                break;
            default:
                qWarning() << "Unknown message type:" << header->type;
        }
        
        // 移除已处理的消息
        receiveBuffer.remove(0, totalSize);
    }
}

void MainWindow::handleResponse(const QByteArray &payload)
{
    if (payload.isEmpty()) {
        return;
    }
    
    const ecn_response_t *resp = 
        reinterpret_cast<const ecn_response_t *>(payload.constData());
    
    if (resp->error_code != ECN_ERR_NONE) {
        handleError(payload);
        return;
    }
    
    // 处理响应数据
    QByteArray data = payload.mid(sizeof(ecn_response_t), resp->data_len);
    
    // TODO: 根据当前状态处理不同类型的响应数据
}

void MainWindow::handleError(const QByteArray &payload)
{
    if (payload.isEmpty()) {
        return;
    }
    
    const ecn_response_t *resp = 
        reinterpret_cast<const ecn_response_t *>(payload.constData());
    
    QString errorMessage;
    switch (resp->error_code) {
        case ECN_ERR_AUTH_FAILED:
            errorMessage = tr("Authentication failed.");
            break;
        case ECN_ERR_USER_EXISTS:
            errorMessage = tr("User already exists.");
            break;
        case ECN_ERR_INVALID_TOKEN:
            errorMessage = tr("Invalid session token.");
            onLogout();  // 强制登出
            break;
        case ECN_ERR_NOT_FOUND:
            errorMessage = tr("Resource not found.");
            break;
        case ECN_ERR_SERVER:
            errorMessage = tr("Server internal error.");
            break;
        case ECN_ERR_INVALID_REQ:
            errorMessage = tr("Invalid request.");
            break;
        default:
            errorMessage = tr("Unknown error occurred.");
    }
    
    QMessageBox::warning(this, tr("Error"), errorMessage);
} 