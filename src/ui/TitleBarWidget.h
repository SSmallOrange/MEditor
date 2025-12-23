#pragma once

#include "ui_TitleBarWidget.h"
#include <QWidget>

class QLabel;
class QPushButton;

// 顶部自定义标题栏：负责显示标题 + 拖动窗口 + 关闭按钮等
class TitleBarWidget : public QWidget
{
	Q_OBJECT
public:
	explicit TitleBarWidget(QWidget* parent = nullptr);

signals:
	void restartRequested();
	void saveRequested();
	void exportRequested();
	void importRequested();
	void minimizeRequested();

protected:
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;

private:
	QPoint m_dragStartPosGlobal;
	QPoint m_windowStartPos;
	bool   m_dragging = false;

	Ui::TitleBarWidget ui;
};