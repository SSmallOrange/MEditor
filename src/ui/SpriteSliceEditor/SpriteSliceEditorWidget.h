#ifndef SPRITESLICEEDITORWIDGET_H
#define SPRITESLICEEDITORWIDGET_H

#include <QWidget>

namespace Ui {
	class SpriteSliceEditorWidget;
}

class SpriteSliceEditorWidget : public QWidget
{
	Q_OBJECT

public:
	explicit SpriteSliceEditorWidget(QWidget* parent = nullptr);
	~SpriteSliceEditorWidget();

	void initSliceTable();

protected:
	bool eventFilter(QObject* watched, QEvent* event) override;

signals:
	void SignalReturnToMainPanel();

private:
	void setupDragOnHeader();

private:
	Ui::SpriteSliceEditorWidget* ui;

	bool m_dragging = false;
	QPoint m_dragStartGlobal;   // 鼠标全局起点
	QPoint m_windowStartTopLeft; // 窗口原始位置
};

#endif // SPRITESLICEEDITORWIDGET_H
