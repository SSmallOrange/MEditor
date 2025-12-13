#pragma once

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include "core/MapDocument.h"
#include "core/TileDragData.h"

class AppContext;
class MapTileItem;

class MapViewWidget : public QGraphicsView
{
	Q_OBJECT
public:
	explicit MapViewWidget(QWidget* parent = nullptr);
	~MapViewWidget();

	void SetContext(AppContext* p) { m_ctx = p; }

	// 更新地图显示
	void updateMap();

	// 属性获取
	int tileWidth() const { return m_tileWidth; }
	int tileHeight() const { return m_tileHeight; }
	int mapWidth() const { return m_mapWidth; }
	int mapHeight() const { return m_mapHeight; }
	bool isGridVisible() const { return m_gridVisible; }
	int currentLayer() const { return m_currentLayer; }
	int zoomPercent() const;

	// 获取当前选中的瓦片
	MapTileItem* selectedTile() const { return m_selectedTile; }

	// 获取所有已放置的瓦片
	const QVector<MapTileItem*>& placedTiles() const { return m_placedTiles; }

	// 清空所有瓦片
	void clearAllTiles();

public slots:
	// 设置属性
	void setGridVisible(bool visible);
	void setGridSize(int width, int height);
	void setMapSize(int width, int height);
	void setCurrentLayer(int layer);
	void setZoomPercent(int percent);

	// 清除选中
	void clearSelection();

	// 删除选中的瓦片
	void deleteSelectedTile();

signals:
	// 属性变化信号
	void gridVisibleChanged(bool visible);
	void gridSizeChanged(int width, int height);
	void mapSizeChanged(int width, int height);
	void zoomChanged(int percent);

	// 瓦片选中信号
	void tileSelected(MapTileItem* tile);
	void tileDeselected();

protected:
	// 拖放事件
	void dragEnterEvent(QDragEnterEvent* event) override;
	void dragMoveEvent(QDragMoveEvent* event) override;
	void dragLeaveEvent(QDragLeaveEvent* event) override;
	void dropEvent(QDropEvent* event) override;

	// 键盘和鼠标事件
	void keyPressEvent(QKeyEvent* event) override;
	void keyReleaseEvent(QKeyEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;

private:
	void setupScene();
	void drawGrid();
	void clearGrid();

	// 高亮相关
	void updateDropHighlight(const QPointF& scenePos, const TileDragData& tileData);
	void updateMoveHighlight(const QPointF& scenePos, MapTileItem* tile);
	void clearDropHighlight();

	// 坐标转换
	QPoint sceneToGrid(const QPointF& scenePos) const;
	QPointF gridToScene(int gridX, int gridY) const;

	// 放置瓦片
	void placeTile(const TileDragData& tileData, const QPoint& gridPos);

	// 更新缩放
	void applyZoom(double scaleFactor);

	// 瓦片点击处理
	void onTileClicked(MapTileItem* tile);

	// 选中瓦片
	void selectTile(MapTileItem* tile);

	// 验证精灵图尺寸是否匹配栅格
	bool validateTileSize(int sliceWidth, int sliceHeight) const;

	// 瓦片拖动处理
	void onTileDragStarted(MapTileItem* tile);
	void onTileDragFinished(MapTileItem* tile, const QPointF& scenePos);

private:
	AppContext* m_ctx = nullptr;
	QGraphicsScene* m_scene = nullptr;

	// 网格线
	QVector<QGraphicsLineItem*> m_gridLines;
	bool m_gridVisible = true;

	// 拖拽高亮
	QGraphicsRectItem* m_dropHighlight = nullptr;
	QVector<QGraphicsRectItem*> m_coverageHighlights;

	// 已放置的瓦片
	QVector<MapTileItem*> m_placedTiles;

	// 当前选中的瓦片
	MapTileItem* m_selectedTile = nullptr;

	// 平移相关
	bool m_spacePressed = false;
	bool m_panning = false;
	QPoint m_lastMousePos;

	// 地图参数
	int m_tileWidth = 32;
	int m_tileHeight = 32;
	int m_mapWidth = 20;
	int m_mapHeight = 15;

	// 图层
	int m_currentLayer = 0;

	// 缩放
	double m_currentScale = 1.0;

	// 瓦片拖动相关
	bool m_tileDragging = false;
	QPointF m_tileOriginalPos;
	int m_tileOriginalGridX = 0;
	int m_tileOriginalGridY = 0;
};