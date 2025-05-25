#include "notelistwidget.h"
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QDateTime>

NoteListWidget::NoteListWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void NoteListWidget::setupUi()
{
    // 创建主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // 创建工具栏布局
    QHBoxLayout *toolbarLayout = new QHBoxLayout();
    mainLayout->addLayout(toolbarLayout);
    
    // 新建笔记按钮
    newNoteButton = new QPushButton(tr("New Note"), this);
    toolbarLayout->addWidget(newNoteButton);
    
    // 刷新按钮
    refreshButton = new QPushButton(tr("Refresh"), this);
    toolbarLayout->addWidget(refreshButton);
    
    // 删除按钮
    deleteButton = new QPushButton(tr("Delete"), this);
    deleteButton->setEnabled(false);
    toolbarLayout->addWidget(deleteButton);
    
    toolbarLayout->addStretch();
    
    // 笔记列表
    noteList = new QListWidget(this);
    mainLayout->addWidget(noteList);
    
    // 状态标签
    statusLabel = new QLabel(this);
    statusLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(statusLabel);
    
    // 连接信号
    connect(newNoteButton, &QPushButton::clicked,
            this, &NoteListWidget::onNewNoteClicked);
    connect(refreshButton, &QPushButton::clicked,
            this, &NoteListWidget::onRefreshClicked);
    connect(deleteButton, &QPushButton::clicked,
            this, &NoteListWidget::onDeleteNoteClicked);
    connect(noteList, &QListWidget::itemClicked,
            this, &NoteListWidget::onNoteItemClicked);
}

void NoteListWidget::updateNoteList(const QVector<NoteInfo> &notes)
{
    noteList->clear();
    
    for (const NoteInfo &note : notes) {
        QListWidgetItem *item = new QListWidgetItem(noteList);
        
        // 设置笔记标题和时间
        QString displayText = QString("%1\n%2")
            .arg(note.title)
            .arg(QDateTime::fromSecsSinceEpoch(note.updated_at)
                 .toString("yyyy-MM-dd HH:mm:ss"));
        
        item->setText(displayText);
        item->setData(Qt::UserRole, note.id);
        
        noteList->addItem(item);
    }
    
    updateButtons();
    
    if (notes.isEmpty()) {
        statusLabel->setText(tr("No notes found"));
    } else {
        statusLabel->setText(tr("%1 notes").arg(notes.size()));
    }
}

void NoteListWidget::onNewNoteClicked()
{
    emit createNewNote();
}

void NoteListWidget::onNoteItemClicked()
{
    QListWidgetItem *item = noteList->currentItem();
    if (item) {
        uint32_t noteId = item->data(Qt::UserRole).toUInt();
        emit noteSelected(noteId);
    }
    updateButtons();
}

void NoteListWidget::onRefreshClicked()
{
    // TODO: 请求更新笔记列表
    statusLabel->setText(tr("Refreshing..."));
}

void NoteListWidget::onDeleteNoteClicked()
{
    QListWidgetItem *item = noteList->currentItem();
    if (!item) {
        return;
    }
    
    uint32_t noteId = item->data(Qt::UserRole).toUInt();
    
    // 确认删除
    if (QMessageBox::question(this, tr("Confirm Delete"),
                            tr("Are you sure you want to delete this note?"),
                            QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        // TODO: 发送删除笔记请求
        statusLabel->setText(tr("Deleting note..."));
    }
}

void NoteListWidget::updateButtons()
{
    bool hasSelection = noteList->currentItem() != nullptr;
    deleteButton->setEnabled(hasSelection);
} 