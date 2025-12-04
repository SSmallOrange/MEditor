#pragma once
#include <QFile>
#include <QWidget>
#include <QDebug>

#define LOAD_QSS(PATH)                                                     \
    do {                                                                                 \
        QString _objName = this->objectName();                                              \
        QString _path = QString(PATH);                               \
        QFile _file(_path);                                                                    \
        if (_file.open(QIODevice::ReadOnly | QIODevice::Text)) {                         \
            QString _qss = QString::fromUtf8(_file.readAll());                           \
            setStyleSheet(_qss);                                              \
            _file.close();                                                               \
            qDebug() << "[QSS] Loaded" << _objName << "from" << _path;             \
        } else {                                                                         \
            qDebug() << "[QSS] Failed to load" << _objName << "attempt path" << _path;       \
        }                                                                                \
    } while (0)

