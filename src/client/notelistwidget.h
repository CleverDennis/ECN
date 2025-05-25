#ifndef NOTELISTWIDGET_H
#define NOTELISTWIDGET_H

#include <QWidget>
#include <QVector>

class QListWidget;
class QPushButton;
class QLabel;

struct NoteInfo {
    uint32_t id;
    QString title;
    qint64 created_at;
    qint64 updated_at;
};

class NoteListWidget : public QWidget {
    Q_OBJECT

public:
    explicit NoteListWidget(QWidget *parent = nullptr);

public slots:
    void updateNoteList(const QVector<NoteInfo> &notes);

signals:
    void noteSelected(uint32_t noteId);
    void createNewNote();

private slots:
    void onNewNoteClicked();
    void onNoteItemClicked();
    void onRefreshClicked();
    void onDeleteNoteClicked();

private:
    void setupUi();
    void updateButtons();

    QListWidget *noteList;
    QPushButton *newNoteButton;
    QPushButton *refreshButton;
    QPushButton *deleteButton;
    QLabel *statusLabel;
};

#endif // NOTELISTWIDGET_H 