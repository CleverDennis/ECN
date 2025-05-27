/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "mainwindow.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MainWindow_t {
    QByteArrayData data[17];
    char stringdata0[174];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MainWindow_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MainWindow_t qt_meta_stringdata_MainWindow = {
    {
QT_MOC_LITERAL(0, 0, 10), // "MainWindow"
QT_MOC_LITERAL(1, 11, 14), // "onLoginSuccess"
QT_MOC_LITERAL(2, 26, 0), // ""
QT_MOC_LITERAL(3, 27, 5), // "token"
QT_MOC_LITERAL(4, 33, 18), // "onNoteListReceived"
QT_MOC_LITERAL(5, 52, 5), // "notes"
QT_MOC_LITERAL(6, 58, 21), // "onNoteContentReceived"
QT_MOC_LITERAL(7, 80, 7), // "content"
QT_MOC_LITERAL(8, 88, 7), // "onError"
QT_MOC_LITERAL(9, 96, 5), // "error"
QT_MOC_LITERAL(10, 102, 15), // "refreshNoteList"
QT_MOC_LITERAL(11, 118, 13), // "createNewNote"
QT_MOC_LITERAL(12, 132, 8), // "editNote"
QT_MOC_LITERAL(13, 141, 6), // "noteId"
QT_MOC_LITERAL(14, 148, 10), // "deleteNote"
QT_MOC_LITERAL(15, 159, 8), // "saveNote"
QT_MOC_LITERAL(16, 168, 5) // "title"

    },
    "MainWindow\0onLoginSuccess\0\0token\0"
    "onNoteListReceived\0notes\0onNoteContentReceived\0"
    "content\0onError\0error\0refreshNoteList\0"
    "createNewNote\0editNote\0noteId\0deleteNote\0"
    "saveNote\0title"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MainWindow[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,   59,    2, 0x08 /* Private */,
       4,    1,   62,    2, 0x08 /* Private */,
       6,    1,   65,    2, 0x08 /* Private */,
       8,    1,   68,    2, 0x08 /* Private */,
      10,    0,   71,    2, 0x08 /* Private */,
      11,    0,   72,    2, 0x08 /* Private */,
      12,    1,   73,    2, 0x08 /* Private */,
      14,    1,   76,    2, 0x08 /* Private */,
      15,    3,   79,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QJsonArray,    5,
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Void, QMetaType::QString,    9,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   13,
    QMetaType::Void, QMetaType::Int,   13,
    QMetaType::Void, QMetaType::Int, QMetaType::QString, QMetaType::QString,   13,   16,    7,

       0        // eod
};

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MainWindow *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->onLoginSuccess((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->onNoteListReceived((*reinterpret_cast< const QJsonArray(*)>(_a[1]))); break;
        case 2: _t->onNoteContentReceived((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 3: _t->onError((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->refreshNoteList(); break;
        case 5: _t->createNewNote(); break;
        case 6: _t->editNote((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 7: _t->deleteNote((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 8: _t->saveNote((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3]))); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_MainWindow.data,
    qt_meta_data_MainWindow,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 9;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
