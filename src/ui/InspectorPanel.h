#pragma once

#include <QWidget>
#include "ui_InspectorPanel.h"

// 右侧 Inspector 面板：使用 .ui 布局
class InspectorPanel : public QWidget
{
	Q_OBJECT
public:
	explicit InspectorPanel(QWidget* parent = nullptr);

	void SetTitle(const QString& title);
	void SetPosition(int x, int y);
	void SetSize(int w, int h);

private:
	Ui::InspectorPanel ui;
};
