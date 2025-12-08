#ifndef SPRITESLICEGRAPHICSVIEW_H
#define SPRITESLICEGRAPHICSVIEW_H

#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QMouseEvent>

class SpriteSliceGraphicsView : public QGraphicsView
{
	Q_OBJECT

public:
	explicit SpriteSliceGraphicsView(QWidget* parent = nullptr);

	void setManualSelectionEnabled(bool enabled);
	bool isManualSelectionEnabled() const { return m_manualSelectionEnabled; }

signals:
	// 框选完成信号，发送场景坐标系下的矩形
	void selectionRectFinished(const QRectF& rect);
	// 鼠标位置变化信号
	void mousePosChanged(const QPointF& scenePos);
	// 点击位置信号（用于选中切片）
	void clickedAt(const QPointF& scenePos);

protected:
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;

private:
	void updateSelectionRect(const QPointF& endPos);

private:
	bool m_manualSelectionEnabled = false;
	bool m_isSelecting = false;
	bool m_isDragging = false;          // 标记是否正在拖拽框选
	QPointF m_selectionStart;           // 场景坐标
	QPointF m_pressPos;                 // 按下时的位置，用于判断是点击还是拖拽
	QGraphicsRectItem* m_selectionRectItem = nullptr;

	static constexpr double DRAG_THRESHOLD = 5.0;  // 拖拽阈值（像素）
};

#endif // SPRITESLICEGRAPHICSVIEW_H