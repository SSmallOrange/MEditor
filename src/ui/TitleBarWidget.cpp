#include "TitleBarWidget.h"

#include <QPainter>
#include <QMouseEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

TitleBarWidget::TitleBarWidget(QWidget* parent)
	: QWidget(parent)
{
	setObjectName("TitleBarWidget");
	ui.setupUi(this);
	setAttribute(Qt::WA_StyledBackground, true);

// 	layout->addWidget(m_titleLabel);
// 	layout->addStretch();
// 	layout->addWidget(m_closeButton);

	QObject::connect(ui.btnClose, &QPushButton::clicked, this, [this]() {
		if (auto w = window())
			w->close();
		});
}

void TitleBarWidget::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);
	QPainter p(this);

	// 更明显一点的浅色渐变
	QLinearGradient g(rect().topLeft(), rect().bottomLeft());
	g.setColorAt(0.0, QColor(252, 253, 255));
	g.setColorAt(1.0, QColor(230, 233, 242));
	p.fillRect(rect(), g);

	// 底部细分割线
	p.setPen(QColor(200, 203, 215));
	p.drawLine(rect().bottomLeft(), rect().bottomRight());
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
