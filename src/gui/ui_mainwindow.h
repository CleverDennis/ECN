/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionLogout;
    QAction *actionExit;
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QListWidget *noteListWidget;
    QTextEdit *noteTextEdit;
    QHBoxLayout *horizontalLayout;
    QPushButton *newNoteButton;
    QPushButton *saveNoteButton;
    QPushButton *deleteNoteButton;
    QMenuBar *menubar;
    QMenu *menuFile;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(800, 600);
        actionLogout = new QAction(MainWindow);
        actionLogout->setObjectName(QString::fromUtf8("actionLogout"));
        actionExit = new QAction(MainWindow);
        actionExit->setObjectName(QString::fromUtf8("actionExit"));
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        noteListWidget = new QListWidget(centralwidget);
        noteListWidget->setObjectName(QString::fromUtf8("noteListWidget"));

        verticalLayout->addWidget(noteListWidget);

        noteTextEdit = new QTextEdit(centralwidget);
        noteTextEdit->setObjectName(QString::fromUtf8("noteTextEdit"));

        verticalLayout->addWidget(noteTextEdit);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        newNoteButton = new QPushButton(centralwidget);
        newNoteButton->setObjectName(QString::fromUtf8("newNoteButton"));

        horizontalLayout->addWidget(newNoteButton);

        saveNoteButton = new QPushButton(centralwidget);
        saveNoteButton->setObjectName(QString::fromUtf8("saveNoteButton"));

        horizontalLayout->addWidget(saveNoteButton);

        deleteNoteButton = new QPushButton(centralwidget);
        deleteNoteButton->setObjectName(QString::fromUtf8("deleteNoteButton"));

        horizontalLayout->addWidget(deleteNoteButton);


        verticalLayout->addLayout(horizontalLayout);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 800, 22));
        menuFile = new QMenu(menubar);
        menuFile->setObjectName(QString::fromUtf8("menuFile"));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        MainWindow->setStatusBar(statusbar);

        menubar->addAction(menuFile->menuAction());
        menuFile->addAction(actionLogout);
        menuFile->addAction(actionExit);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "ECN - Encrypted Cloud Notes", nullptr));
        actionLogout->setText(QCoreApplication::translate("MainWindow", "Logout", nullptr));
        actionExit->setText(QCoreApplication::translate("MainWindow", "Exit", nullptr));
        newNoteButton->setText(QCoreApplication::translate("MainWindow", "New Note", nullptr));
        saveNoteButton->setText(QCoreApplication::translate("MainWindow", "Save Note", nullptr));
        deleteNoteButton->setText(QCoreApplication::translate("MainWindow", "Delete Note", nullptr));
        menuFile->setTitle(QCoreApplication::translate("MainWindow", "File", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
