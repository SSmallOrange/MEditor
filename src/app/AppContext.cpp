#include "AppContext.h"
#include <QFile>
#include <QApplication>
#include <QDebug>

void AppContext::loadStyle(const QString& qssPath)
{
	QFile f(qssPath);
	if (!f.open(QFile::ReadOnly | QFile::Text)) {
		qWarning() << "Failed to load style:" << qssPath;
		return;
	}
	const QString style = QString::fromUtf8(f.readAll());
// 	qDebug() << "[Style] length =" << style.size();
// 	qDebug().noquote() << "[Style] head =" << style.left(200);
	qApp->setStyleSheet(style);
}
