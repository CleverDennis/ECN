#ifndef NOTEEDITWIDGET_H
#define NOTEEDITWIDGET_H

#include <QWidget>
#include <QByteArray>

class QLineEdit;
class QTextEdit;
class QPushButton;
class QLabel;

class NoteEditWidget : public QWidget {
    Q_OBJECT

public:
    explicit NoteEditWidget(QWidget *parent = nullptr);

public slots:
    void loadNote(uint32_t noteId);
    void newNote();

signals:
    void noteSaved();
    void backToList();

private slots:
    void onSaveClicked();
    void onBackClicked();
    void onContentChanged();

private:
    void setupUi();
    void clearInputs();
    QByteArray encryptContent(const QString &content);
    QString decryptContent(const QByteArray &encrypted);

    QLineEdit *titleEdit;
    QTextEdit *contentEdit;
    QPushButton *saveButton;
    QPushButton *backButton;
    QLabel *statusLabel;

    uint32_t currentNoteId;
    bool isNewNote;
    QByteArray noteKey;  // SM4密钥
};

#endif // NOTEEDITWIDGET_H 