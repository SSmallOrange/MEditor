#include "TitleBarWidget.h"

#include <QMouseEvent>

TitleBarWidget::TitleBarWidget(QWidget* parent)
	: QWidget(parent)
{
	setObjectName("TitleBarWidget");
	setAttribute(Qt::WA_StyledBackground, true);
	ui.setupUi(this);

	QObject::connect(ui.btnClose, &QPushButton::clicked, this, [this]() {
		if (auto w = window())
			w->close();
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
