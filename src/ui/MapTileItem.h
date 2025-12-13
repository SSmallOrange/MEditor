#pragma once

#include <QGraphicsPixmapItem>
#include <QObject>
#include "core/SpriteSliceDefine.h"

class MapViewWidget;

// 地图上的瓦片图元
class MapTileItem : public QObject, public QGraphicsPixmapItem
{
	Q_OBJECT
public:
	MapTileItem(const QPixmap& pixmap, const SpriteSlice& slice,
		int gridX, int gridY, int layer, QGraphicsItem* parent = nullptr);

	// 网格坐标
	int gridX() const { return m_gridX; }
	int gridY() const { return m_gridY; }
	void setGridPos(int x, int y);

	// 切片数据
	const SpriteSlice& slice() const { return m_slice; }
	QString tilesetId() const { return m_tilesetId; }
	void setTilesetId(const QString& id) { m_tilesetId = id; }

	// 占用的网格数量
	int gridWidth() const { return m_gridWidth; }
	int gridHeight() const { return m_gridHeight; }
	void setGridSize(int w, int h) { m_gridWidth = w; m_gridHeight = h; }

	// 图层
	int layer() const { return m_layer; }
	void setLayer(int layer) { m_layer = layer; }

	// 选中状态
	bool isSelected() const { return m_selected; }
	void setSelected(bool selected);

	// 拖动状态
	bool isDragging() const { return m_dragging; }

signals:
	void clicked(MapTileItem* item);
	void selectionChanged(MapTileItem* item, bool selected);
	void dragStarted(MapTileItem* item);
	void dragFinished(MapTileItem* item, const QPointF& scenePos);

protected:
	void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
	void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
	void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
	SpriteSlice m_slice;
	QString m_tilesetId;
	int m_gridX = 0;
	int m_gridY = 0;
	int m_gridWidth = 1;
	int m_gridHeight = 1;
	int m_layer = 0;
	bool m_selected = false;

	// 拖动相关
	bool m_dragging = false;
	QPointF m_dragStartPos;
	QPointF m_originalPos;
};