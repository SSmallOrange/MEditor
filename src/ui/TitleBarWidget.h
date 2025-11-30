#pragma once

#include "ui_TitleBarWidget.h"

#include <QWidget>

class QLabel;
class QPushButton;

// 顶部自定义标题栏（负责显示标题 + 拖动窗口 + 关闭按钮）
class TitleBarWidget : public QWidget
{
public:
	explicit TitleBarWidget(QWidget* parent = nullptr);

protected:
	void paintEvent(QPaintEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;

private:
	QPoint m_dragStartPosGlobal;  // 鼠标按下时的全局坐标
	QPoint m_windowStartPos;      // 鼠标按下时窗口左上角坐标
	bool   m_dragging = false;

	Ui::TitleBarWidget ui;
};
