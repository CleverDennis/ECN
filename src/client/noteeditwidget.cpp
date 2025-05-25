#include "noteeditwidget.h"
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include "../../include/ecn_crypto.h"

NoteEditWidget::NoteEditWidget(QWidget *parent)
    : QWidget(parent)
    , currentNoteId(0)
    , isNewNote(false)
{
    setupUi();
}

void NoteEditWidget::setupUi()
{
    // 创建主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // 创建工具栏布局
    QHBoxLayout *toolbarLayout = new QHBoxLayout();
    mainLayout->addLayout(toolbarLayout);
    
    // 返回按钮
    backButton = new QPushButton(tr("Back"), this);
    toolbarLayout->addWidget(backButton);
    
    // 保存按钮
    saveButton = new QPushButton(tr("Save"), this);
    saveButton->setEnabled(false);
    toolbarLayout->addWidget(saveButton);
    
    toolbarLayout->addStretch();
    
    // 标题输入
    titleEdit = new QLineEdit(this);
    titleEdit->setPlaceholderText(tr("Enter note title"));
    mainLayout->addWidget(titleEdit);
    
    // 内容编辑器
    contentEdit = new QTextEdit(this);
    contentEdit->setPlaceholderText(tr("Enter note content"));
    mainLayout->addWidget(contentEdit);
    
    // 状态标签
    statusLabel = new QLabel(this);
    statusLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(statusLabel);
    
    // 连接信号
    connect(backButton, &QPushButton::clicked,
            this, &NoteEditWidget::onBackClicked);
    connect(saveButton, &QPushButton::clicked,
            this, &NoteEditWidget::onSaveClicked);
    connect(titleEdit, &QLineEdit::textChanged,
            this, &NoteEditWidget::onContentChanged);
    connect(contentEdit, &QTextEdit::textChanged,
            this, &NoteEditWidget::onContentChanged);
}

void NoteEditWidget::loadNote(uint32_t noteId)
{
    currentNoteId = noteId;
    isNewNote = false;
    
    // TODO: 从服务器获取笔记内容
    // 临时测试数据
    titleEdit->setText("Test Note");
    contentEdit->setPlainText("This is a test note content.");
    noteKey = QByteArray(16, 0);  // 临时密钥
    
    saveButton->setEnabled(false);
    statusLabel->clear();
}

void NoteEditWidget::newNote()
{
    currentNoteId = 0;
    isNewNote = true;
    
    clearInputs();
    
    // 生成新的SM4密钥
    uint8_t key[16];
    if (!RAND_bytes(key, sizeof(key))) {
        QMessageBox::critical(this, tr("Error"),
                            tr("Failed to generate encryption key."));
        return;
    }
    noteKey = QByteArray((char*)key, sizeof(key));
    
    saveButton->setEnabled(false);
    statusLabel->clear();
}

void NoteEditWidget::onSaveClicked()
{
    QString title = titleEdit->text().trimmed();
    QString content = contentEdit->toPlainText();
    
    if (title.isEmpty()) {
        QMessageBox::warning(this, tr("Error"),
                           tr("Please enter a title for the note."));
        return;
    }
    
    // 加密笔记内容
    QByteArray encryptedContent = encryptContent(content);
    if (encryptedContent.isEmpty()) {
        return;
    }
    
    // TODO: 发送保存请求到服务器
    statusLabel->setText(tr("Saving..."));
    
    // 临时：模拟保存成功
    QMessageBox::information(this, tr("Success"),
                           tr("Note saved successfully."));
    emit noteSaved();
}

void NoteEditWidget::onBackClicked()
{
    if (saveButton->isEnabled()) {
        // 有未保存的更改
        if (QMessageBox::question(this, tr("Unsaved Changes"),
                                tr("You have unsaved changes. Do you want to discard them?"),
                                QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
            return;
        }
    }
    
    emit backToList();
}

void NoteEditWidget::onContentChanged()
{
    saveButton->setEnabled(true);
    statusLabel->clear();
}

void NoteEditWidget::clearInputs()
{
    titleEdit->clear();
    contentEdit->clear();
    statusLabel->clear();
}

QByteArray NoteEditWidget::encryptContent(const QString &content)
{
    QByteArray data = content.toUtf8();
    QByteArray encrypted(data.size() + 16, 0);  // 预留IV空间
    
    if (ecn_sm4_encrypt_ctr((uint8_t*)data.data(), data.size(),
                           (uint8_t*)noteKey.data(),
                           (uint8_t*)encrypted.data()) != 0) {
        QMessageBox::critical(this, tr("Error"),
                            tr("Failed to encrypt note content."));
        return QByteArray();
    }
    
    return encrypted;
}

QString NoteEditWidget::decryptContent(const QByteArray &encrypted)
{
    if (encrypted.size() <= 16) {  // 至少要有IV
        return QString();
    }
    
    QByteArray decrypted(encrypted.size() - 16, 0);
    
    if (ecn_sm4_decrypt_ctr((uint8_t*)encrypted.data(), encrypted.size(),
                           (uint8_t*)noteKey.data(),
                           (uint8_t*)decrypted.data()) != 0) {
        QMessageBox::critical(this, tr("Error"),
                            tr("Failed to decrypt note content."));
        return QString();
    }
    
    return QString::fromUtf8(decrypted);
}