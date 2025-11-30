#pragma once
#include "core/MapDocument.h"

#include <QObject>
#include <QWidget>

class AppContext;

// 地图视图：后续会负责绘制 tiles、处理鼠标事件
class MapViewWidget : public QWidget
{
	Q_OBJECT
public:
	explicit MapViewWidget(QWidget* parent = nullptr);

	void SetContext(AppContext* p) { m_ctx = p; }
protected:
	void paintEvent(QPaintEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void keyPressEvent(QKeyEvent* event) override;
	void keyReleaseEvent(QKeyEvent* event) override;

private:
	QPoint TileFromPos(const QPoint& pos) const;    // 屏幕坐标 → tile 坐标
	QRect  TileRect(const QPoint& tile, const MapDocument* doc, const QRect& gridArea) const;   // tile 坐标 → 屏幕矩形

private:
	AppContext* m_ctx = nullptr;

	// 视图偏移（平移网格），单位：像素
	QPoint m_viewOffset = QPoint(0, 0);

	// 拖动相关
	bool   m_spacePressed = false;   // 是否按着空格
	bool   m_panning = false;   // 是否正在拖动
	QPoint m_lastMousePos;

	static constexpr int kMargin = 20;   // 网格区域与窗口边的间距

	QPoint m_selectedTile = QPoint(-1, -1);   // 选中的格子，(-1,-1) 表示无
};