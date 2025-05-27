QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ECN
TEMPLATE = app

INCLUDEPATH += ../../include

LIBS += -lsqlite3 -lgmssl

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    logindialog.cpp

HEADERS += \
    mainwindow.h \
    logindialog.h \
    crypto_utils.h

FORMS += \
    mainwindow.ui \
    logindialog.ui

CONFIG += c++17

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target 