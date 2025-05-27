#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QTextEdit>
#include <QDialog>
#include <QTimer>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onLoginSuccess(const QString &token);
    void onNoteListReceived(const QJsonArray &notes);
    void onNoteContentReceived(const QString &content);
    void onError(const QString &error);
    void refreshNoteList();
    void createNewNote();
    void editNote(int noteId);
    void deleteNote(int noteId);
    void saveNote(int noteId, const QString &title, const QString &content);

private:
    void setupUI();
    void setupConnections();
    void showLoginDialog();
    void showNoteList();
    void showNoteEditor(int noteId = -1);

    QNetworkAccessManager *networkManager;
    QString sessionToken;
    QListWidget *noteListWidget;
    QTextEdit *noteEditor;
    QLineEdit *noteTitleEdit;
    QPushButton *newNoteButton;
    QPushButton *saveButton;
    QPushButton *deleteButton;
    QTimer *refreshTimer;
    int currentNoteId;
};

#endif // MAINWINDOW_H 