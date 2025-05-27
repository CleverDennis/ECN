#include "mainwindow.h"
#include "logindialog.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , networkManager(new QNetworkAccessManager(this))
    , currentNoteId(-1)
{
    setupUI();
    setupConnections();
    showLoginDialog();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    setWindowTitle("加密云笔记");
    resize(800, 600);

    // 创建中央部件
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // 创建布局
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

    // 左侧笔记列表
    QWidget *leftWidget = new QWidget;
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    noteListWidget = new QListWidget;
    newNoteButton = new QPushButton("新建笔记");
    leftLayout->addWidget(noteListWidget);
    leftLayout->addWidget(newNoteButton);

    // 右侧笔记编辑区
    QWidget *rightWidget = new QWidget;
    QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);
    noteTitleEdit = new QLineEdit;
    noteEditor = new QTextEdit;
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    saveButton = new QPushButton("保存");
    deleteButton = new QPushButton("删除");
    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(deleteButton);
    rightLayout->addWidget(noteTitleEdit);
    rightLayout->addWidget(noteEditor);
    rightLayout->addLayout(buttonLayout);

    // 添加到主布局
    mainLayout->addWidget(leftWidget, 1);
    mainLayout->addWidget(rightWidget, 2);

    // 创建定时刷新器
    refreshTimer = new QTimer(this);
    refreshTimer->setInterval(30000); // 30秒刷新一次
}

void MainWindow::setupConnections()
{
    connect(newNoteButton, &QPushButton::clicked, this, &MainWindow::createNewNote);
    connect(saveButton, &QPushButton::clicked, [this]() {
        saveNote(currentNoteId, noteTitleEdit->text(), noteEditor->toPlainText());
    });
    connect(deleteButton, &QPushButton::clicked, [this]() {
        if (currentNoteId != -1) {
            deleteNote(currentNoteId);
        }
    });
    connect(noteListWidget, &QListWidget::itemDoubleClicked, [this](QListWidgetItem *item) {
        int noteId = item->data(Qt::UserRole).toInt();
        editNote(noteId);
    });
    connect(refreshTimer, &QTimer::timeout, this, &MainWindow::refreshNoteList);
}

void MainWindow::showLoginDialog()
{
    LoginDialog *loginDialog = new LoginDialog(this);
    connect(loginDialog, &LoginDialog::loginSuccess, this, &MainWindow::onLoginSuccess);
    loginDialog->exec();
}

void MainWindow::onLoginSuccess(const QString &token)
{
    sessionToken = token;
    showNoteList();
    refreshTimer->start();
}

void MainWindow::showNoteList()
{
    QNetworkRequest request(QUrl("http://localhost:8443/api/notes"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", sessionToken.toUtf8());

    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            onNoteListReceived(doc.array());
        } else {
            onError(reply->errorString());
        }
        reply->deleteLater();
    });
}

void MainWindow::onNoteListReceived(const QJsonArray &notes)
{
    noteListWidget->clear();
    for (const QJsonValue &note : notes) {
        QJsonObject noteObj = note.toObject();
        QListWidgetItem *item = new QListWidgetItem(noteObj["title"].toString());
        item->setData(Qt::UserRole, noteObj["id"].toInt());
        noteListWidget->addItem(item);
    }
}

void MainWindow::onNoteContentReceived(const QString &content)
{
    noteEditor->setText(content);
}

void MainWindow::refreshNoteList()
{
    showNoteList();
}

void MainWindow::createNewNote()
{
    currentNoteId = -1;
    noteTitleEdit->clear();
    noteEditor->clear();
    noteTitleEdit->setFocus();
}

void MainWindow::editNote(int noteId)
{
    currentNoteId = noteId;
    QNetworkRequest request(QUrl(QString("http://localhost:8443/api/notes/%1").arg(noteId)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", sessionToken.toUtf8());

    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QJsonObject note = doc.object();
            noteTitleEdit->setText(note["title"].toString());
            noteEditor->setText(note["content"].toString());
        } else {
            onError(reply->errorString());
        }
        reply->deleteLater();
    });
}

void MainWindow::saveNote(int noteId, const QString &title, const QString &content)
{
    QJsonObject note;
    note["title"] = title;
    note["content"] = content;

    QNetworkRequest request(QUrl(noteId == -1 ? 
        "http://localhost:8443/api/notes" : 
        QString("http://localhost:8443/api/notes/%1").arg(noteId)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", sessionToken.toUtf8());

    QNetworkReply *reply = networkManager->post(request, QJsonDocument(note).toJson());
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            refreshNoteList();
        } else {
            onError(reply->errorString());
        }
        reply->deleteLater();
    });
}

void MainWindow::deleteNote(int noteId)
{
    QNetworkRequest request(QUrl(QString("http://localhost:8443/api/notes/%1").arg(noteId)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", sessionToken.toUtf8());

    QNetworkReply *reply = networkManager->deleteResource(request);
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            refreshNoteList();
            createNewNote();
        } else {
            onError(reply->errorString());
        }
        reply->deleteLater();
    });
}

void MainWindow::onError(const QString &error)
{
    QMessageBox::critical(this, "错误", error);
} 