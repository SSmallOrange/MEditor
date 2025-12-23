#include "TitleBarWidget.h"

#include <QMouseEvent>

TitleBarWidget::TitleBarWidget(QWidget* parent)
	: QWidget(parent)
{
	setObjectName("TitleBarWidget");
	setAttribute(Qt::WA_StyledBackground, true);
	ui.setupUi(this);

	// 关闭按钮
	QObject::connect(ui.btnClose, &QPushButton::clicked, this, [this]() {
		if (auto w = window())
			w->close();
	});

	// 最小化按钮
	QObject::connect(ui.btnMinimize, &QPushButton::clicked, this, [this]() {
		emit minimizeRequested();
		if (auto w = window())
			w->showMinimized();
	});

	// 重置按钮
	QObject::connect(ui.btnRestart, &QPushButton::clicked, this, [this]() {
		emit restartRequested();
	});

	// 保存按钮
	QObject::connect(ui.btnSave, &QPushButton::clicked, this, [this]() {
		emit saveRequested();
	});

	// 导出按钮
	QObject::connect(ui.btnExport, &QPushButton::clicked, this, [this]() {
		emit importRequested();
	});

	// 导入按钮
	QObject::connect(ui.btnImport, &QPushButton::clicked, this, [this]() {
		emit exportRequested();
	});
}

void TitleBarWidget::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) {
		m_dragging = true;
		m_dragStartPosGlobal = event->globalPosition().toPoint();
		if (auto w = window())
			m_windowStartPos = w->pos();
		event->accept();
	}
	else {
		QWidget::mousePressEvent(event);
	}
}

void TitleBarWidget::mouseMoveEvent(QMouseEvent* event)
{
	if (m_dragging) {
		const QPoint delta = event->globalPosition().toPoint() - m_dragStartPosGlobal;
		if (auto w = window())
			w->move(m_windowStartPos + delta);
		event->accept();
	}
	else {
		QWidget::mouseMoveEvent(event);
	}
}

void TitleBarWidget::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton && m_dragging) {
		m_dragging = false;
		event->accept();
	}
	else {
		QWidget::mouseReleaseEvent(event);
	}
}