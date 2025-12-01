#pragma once
#include <QFile>
#include <QWidget>
#include <QDebug>

/*
 * 基础策略：
 * 1. 目标文件名：类名 + ".qss"，例如 TilesetsPanel → TilesetsPanel.qss
 * 2. 优先资源路径(:/qss/类名.qss)，若不存在回退为运行目录下 qss/类名.qss
 * 3. LOAD_* 覆盖现有样式；APPEND_* 追加到现有样式末尾
 * 4. 若需按对象真实运行时类（包含继承）加载，可使用 LOAD_SELF_QSS / APPEND_SELF_QSS
 */

 /* 覆盖加载：指定编译期类名 */
#define LOAD_QSS_FOR(CLASS, OBJ_PTR)                                                     \
    do {                                                                                 \
        const char* _cls = #CLASS;                                                       \
        QString _path = QString(":/qss/%1.qss").arg(_cls);                               \
        QFile _file(_path);                                                              \
        if (!_file.exists()) {                                                           \
            _path = QString("qss/%1.qss").arg(_cls);                                     \
            _file.setFileName(_path);                                                    \
        }                                                                                \
        if (_file.open(QIODevice::ReadOnly | QIODevice::Text)) {                         \
            QString _qss = QString::fromUtf8(_file.readAll());                           \
            (OBJ_PTR)->setStyleSheet(_qss);                                              \
            _file.close();                                                               \
            /* 调试输出 */                                                               \
            qDebug() << "[QSS] Loaded (replace)" << _cls << "from" << _path;             \
        } else {                                                                         \
            qDebug() << "[QSS] Failed to load" << _cls << "attempt path" << _path;       \
        }                                                                                \
    } while (0)

/* 追加加载：指定编译期类名 */
#define APPEND_QSS_FOR(CLASS, OBJ_PTR)                                                   \
    do {                                                                                 \
        const char* _cls = #CLASS;                                                       \
        QString _path = QString(":/qss/%1.qss").arg(_cls);                               \
        QFile _file(_path);                                                              \
        if (!_file.exists()) {                                                           \
            _path = QString("qss/%1.qss").arg(_cls);                                     \
            _file.setFileName(_path);                                                    \
        }                                                                                \
        if (_file.open(QIODevice::ReadOnly | QIODevice::Text)) {                         \
            QString _qss = QString::fromUtf8(_file.readAll());                           \
            QString _merged = (OBJ_PTR)->styleSheet();                                   \
            if (!_merged.isEmpty()) { _merged += "\n"; }                                 \
            _merged += _qss;                                                             \
            (OBJ_PTR)->setStyleSheet(_merged);                                           \
            _file.close();                                                               \
            qDebug() << "[QSS] Loaded (append)" << _cls << "from" << _path;              \
        } else {                                                                         \
            qDebug() << "[QSS] Failed to append" << _cls << "attempt path" << _path;     \
        }                                                                                \
    } while (0)

/* 覆盖加载：使用对象真实运行时类名（含继承） */
#define LOAD_SELF_QSS(OBJ_PTR)                                                           \
    do {                                                                                 \
        const char* _cls = (OBJ_PTR)->metaObject()->className();                         \
        QString _path = QString(":/qss/%1.qss").arg(_cls);                               \
        QFile _file(_path);                                                              \
        if (!_file.exists()) {                                                           \
            _path = QString("qss/%1.qss").arg(_cls);                                     \
            _file.setFileName(_path);                                                    \
        }                                                                                \
        if (_file.open(QIODevice::ReadOnly | QIODevice::Text)) {                         \
            (OBJ_PTR)->setStyleSheet(QString::fromUtf8(_file.readAll()));                \
            _file.close();                                                               \
            qDebug() << "[QSS] Loaded (self replace)" << _cls << "from" << _path;        \
        } else {                                                                         \
            qDebug() << "[QSS] Failed self load" << _cls << "attempt path" << _path;     \
        }                                                                                \
    } while (0)

/* 追加加载：使用对象真实运行时类名 */
#define APPEND_SELF_QSS(OBJ_PTR)                                                         \
    do {                                                                                 \
        const char* _cls = (OBJ_PTR)->metaObject()->className();                         \
        QString _path = QString(":/qss/%1.qss").arg(_cls);                               \
        QFile _file(_path);                                                              \
        if (!_file.exists()) {                                                           \
            _path = QString("qss/%1.qss").arg(_cls);                                     \
            _file.setFileName(_path);                                                    \
        }                                                                                \
        if (_file.open(QIODevice::ReadOnly | QIODevice::Text)) {                         \
            QString _qss = QString::fromUtf8(_file.readAll());                           \
            QString _merged = (OBJ_PTR)->styleSheet();                                   \
            if (!_merged.isEmpty()) { _merged += "\n"; }                                 \
            _merged += _qss;                                                             \
            (OBJ_PTR)->setStyleSheet(_merged);                                           \
            _file.close();                                                               \
            qDebug() << "[QSS] Loaded (self append)" << _cls << "from" << _path;         \
        } else {                                                                         \
            qDebug() << "[QSS] Failed self append" << _cls << "attempt path" << _path;   \
        }                                                                                \
    } while (0)