#include "MapViewWidget.h"
#include "MapTileItem.h"
#include "app/AppContext.h"
#include "app/DocumentManager.h"
#include "core/TileDragData.h"

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QKeyEvent>
#include <QScrollBar>
#include <QtMath>

MapViewWidget::MapViewWidget(QWidget* parent)
	: QGraphicsView(parent)
	, m_ctx(nullptr)
{
	setObjectName("mapViewWidget");
	setAutoFillBackground(false);
	setMouseTracking(true);
	setFocusPolicy(Qt::ClickFocus);

	// 启用拖放
	setAcceptDrops(true);

	// 设置渲染选项
	setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
	setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
	setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
	setResizeAnchor(QGraphicsView::AnchorViewCenter);

	// 隐藏滚动条
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	setupScene();
}

MapViewWidget::~MapViewWidget()
{
}

void MapViewWidget::setupScene()
{
	m_scene = new QGraphicsScene(this);
	setScene(m_scene);

	// 设置背景色
	m_scene->setBackgroundBrush(QBrush(QColor(249, 250, 252)));

	// 创建高亮矩形（初始隐藏）
	m_dropHighlight = new QGraphicsRectItem();
	m_dropHighlight->setPen(QPen(QColor(80, 140, 255), 2));
	m_dropHighlight->setBrush(QBrush(QColor(80, 140, 255, 60)));
	m_dropHighlight->setZValue(1000);
	m_dropHighlight->setVisible(false);
	m_scene->addItem(m_dropHighlight);

	// 初始化网格
	drawGrid();
}

void MapViewWidget::updateMap()
{
	if (!m_ctx)
		return;

	const MapDocument* doc = m_ctx->documentManager.document();
	if (!doc)
		return;

	// 更新地图参数
	m_tileWidth = doc->tileWidth;
	m_tileHeight = doc->tileHeight;
	m_mapWidth = doc->width;
	m_mapHeight = doc->height;

	// 重绘网格
	drawGrid();

	// 设置场景大小
	m_scene->setSceneRect(0, 0, m_mapWidth * m_tileWidth, m_mapHeight * m_tileHeight);

	// 发送信号通知 UI 更新
	emit gridSizeChanged(m_tileWidth, m_tileHeight);
	emit mapSizeChanged(m_mapWidth, m_mapHeight);
}

void MapViewWidget::drawGrid()
{
	clearGrid();

	int totalWidth = m_mapWidth * m_tileWidth;
	int totalHeight = m_mapHeight * m_tileHeight;

	if (m_gridVisible)
	{
		QPen gridPen(QColor(220, 220, 225), 1);

		// 垂直线
		for (int col = 0; col <= m_mapWidth; ++col)
		{
			int x = col * m_tileWidth;
			auto* line = m_scene->addLine(x, 0, x, totalHeight, gridPen);
			line->setZValue(1);
			m_gridLines.append(line);
		}

		// 水平线
		for (int row = 0; row <= m_mapHeight; ++row)
		{
			int y = row * m_tileHeight;
			auto* line = m_scene->addLine(0, y, totalWidth, y, gridPen);
			line->setZValue(1);
			m_gridLines.append(line);
		}
	}

	// 外边框始终显示
	QPen borderPen(QColor(190, 190, 195), 2);
	auto* border = m_scene->addRect(0, 0, totalWidth, totalHeight, borderPen);
	border->setZValue(2);
	m_gridLines.append(reinterpret_cast<QGraphicsLineItem*>(border));

	// 更新场景矩形
	m_scene->setSceneRect(-50, -50, totalWidth + 100, totalHeight + 100);
}

void MapViewWidget::clearGrid()
{
	for (auto* item : m_gridLines)
	{
		m_scene->removeItem(item);
		delete item;
	}
	m_gridLines.clear();
}

// ============== 尺寸验证 ==============

bool MapViewWidget::validateTileSize(int sliceWidth, int sliceHeight) const
{
	// 精灵图尺寸必须是栅格尺寸的整数倍
	if (m_tileWidth <= 0 || m_tileHeight <= 0)
		return false;

	return (sliceWidth % m_tileWidth == 0) && (sliceHeight % m_tileHeight == 0);
}

// ============== 属性设置 ==============

void MapViewWidget::setGridVisible(bool visible)
{
	if (m_gridVisible == visible)
		return;

	m_gridVisible = visible;
	drawGrid();
	emit gridVisibleChanged(visible);
}

void MapViewWidget::setGridSize(int width, int height)
{
	if (m_tileWidth == width && m_tileHeight == height)
		return;

	m_tileWidth = width;
	m_tileHeight = height;

	// 更新 MapDocument
	if (m_ctx)
	{
		MapDocument* doc = const_cast<MapDocument*>(m_ctx->documentManager.document());
		if (doc)
		{
			doc->tileWidth = width;
			doc->tileHeight = height;
		}
	}

	drawGrid();
	emit gridSizeChanged(width, height);
}

void MapViewWidget::setMapSize(int width, int height)
{
	if (m_mapWidth == width && m_mapHeight == height)
		return;

	m_mapWidth = width;
	m_mapHeight = height;

	// 更新 MapDocument
	if (m_ctx)
	{
		MapDocument* doc = const_cast<MapDocument*>(m_ctx->documentManager.document());
		if (doc)
		{
			doc->width = width;
			doc->height = height;
		}
	}

	drawGrid();
	m_scene->setSceneRect(-50, -50, m_mapWidth * m_tileWidth + 100, m_mapHeight * m_tileHeight + 100);
	emit mapSizeChanged(width, height);
}

void MapViewWidget::setCurrentLayer(int layer)
{
	if (m_currentLayer == layer)
		return;

	m_currentLayer = layer;

	// 切换图层时，如果当前选中的瓦片不在新图层，则取消选中
	if (m_selectedTile && m_selectedTile->layer() != layer)
	{
		clearSelection();
	}

	qDebug() << "Current layer changed to:" << layer;
}

int MapViewWidget::zoomPercent() const
{
	return static_cast<int>(m_currentScale * 100);
}

void MapViewWidget::setZoomPercent(int percent)
{
	double newScale = percent / 100.0;
	if (qFuzzyCompare(m_currentScale, newScale))
		return;

	resetTransform();
	m_currentScale = newScale;
	scale(m_currentScale, m_currentScale);

	emit zoomChanged(percent);
}

void MapViewWidget::applyZoom(double scaleFactor)
{
	m_currentScale *= scaleFactor;
	m_currentScale = qBound(0.1, m_currentScale, 4.0);

	resetTransform();
	scale(m_currentScale, m_currentScale);

	emit zoomChanged(static_cast<int>(m_currentScale * 100));
}

// ============== 选中相关 ==============

void MapViewWidget::clearSelection()
{
	if (m_selectedTile)
	{
		m_selectedTile->setSelected(false);
		m_selectedTile = nullptr;
		emit tileDeselected();
	}
}

void MapViewWidget::selectTile(MapTileItem* tile)
{
	if (m_selectedTile == tile)
		return;

	// 取消之前的选中
	if (m_selectedTile)
	{
		m_selectedTile->setSelected(false);
	}

	m_selectedTile = tile;

	if (tile)
	{
		tile->setSelected(true);
		emit tileSelected(tile);
		qDebug() << "Selected tile:" << tile->slice().name
			<< "at grid:" << tile->gridX() << "," << tile->gridY()
			<< "layer:" << tile->layer();
	}
	else
	{
		emit tileDeselected();
	}
}

void MapViewWidget::onTileClicked(MapTileItem* tile)
{
	if (!tile)
		return;

	// 检查图层：只有当前图层的瓦片才能被选中
	if (tile->layer() != m_currentLayer)
	{
		qDebug() << "Cannot select tile: tile layer" << tile->layer()
			<< "!= current layer" << m_currentLayer;
		return;
	}

	selectTile(tile);
}

void MapViewWidget::deleteSelectedTile()
{
	if (!m_selectedTile)
		return;

	// 从列表中移除
	m_placedTiles.removeOne(m_selectedTile);

	// 从场景中移除
	m_scene->removeItem(m_selectedTile);

	// 删除对象
	m_selectedTile->deleteLater();
	m_selectedTile = nullptr;

	emit tileDeselected();

	qDebug() << "Deleted selected tile";
}

// ============== 瓦片拖动处理 ==============

void MapViewWidget::onTileDragStarted(MapTileItem* tile)
{
	if (!tile || tile != m_selectedTile)
		return;

	// 检查尺寸是否匹配
	if (!validateTileSize(tile->slice().width, tile->slice().height))
	{
		qDebug() << "Cannot drag tile: size mismatch";
		return;
	}

	m_tileDragging = true;
	m_tileOriginalPos = tile->pos();
	m_tileOriginalGridX = tile->gridX();
	m_tileOriginalGridY = tile->gridY();

	qDebug() << "Tile drag started";
}

void MapViewWidget::onTileDragFinished(MapTileItem* tile, const QPointF& scenePos)
{
	if (!tile || !m_tileDragging)
	{
		// 如果没有在拖动状态，恢复位置
		if (tile)
		{
			tile->setPos(gridToScene(tile->gridX(), tile->gridY()));
		}
		return;
	}

	m_tileDragging = false;

	// 检查尺寸是否匹配
	if (!validateTileSize(tile->slice().width, tile->slice().height))
	{
		// 尺寸不匹配，恢复原位置
		tile->setPos(m_tileOriginalPos);
		qDebug() << "Tile drag failed: size mismatch, restoring position";
		return;
	}

	// 计算新的网格位置
	QPoint newGridPos = sceneToGrid(scenePos);

	// 检查边界
	int gridW = tile->gridWidth();
	int gridH = tile->gridHeight();

	if (newGridPos.x() < 0 || newGridPos.y() < 0 ||
		newGridPos.x() + gridW > m_mapWidth || newGridPos.y() + gridH > m_mapHeight)
	{
		// 超出边界，恢复原位置
		tile->setPos(m_tileOriginalPos);
		tile->setGridPos(m_tileOriginalGridX, m_tileOriginalGridY);
		qDebug() << "Tile drag failed: out of bounds";
	}
	else
	{
		// 放置到新位置（对齐网格）
		tile->setGridPos(newGridPos.x(), newGridPos.y());
		tile->setPos(gridToScene(newGridPos.x(), newGridPos.y()));
		qDebug() << "Tile moved to grid:" << newGridPos;
	}

	clearDropHighlight();
}

void MapViewWidget::updateMoveHighlight(const QPointF& scenePos, MapTileItem* tile)
{
	if (!tile)
	{
		clearDropHighlight();
		return;
	}

	QPoint gridPos = sceneToGrid(scenePos);

	int gridW = tile->gridWidth();
	int gridH = tile->gridHeight();

	// 检查边界
	if (gridPos.x() < 0 || gridPos.y() < 0 ||
		gridPos.x() + gridW > m_mapWidth || gridPos.y() + gridH > m_mapHeight)
	{
		clearDropHighlight();
		return;
	}

	// 清除旧的高亮
	for (auto* rect : m_coverageHighlights)
	{
		m_scene->removeItem(rect);
		delete rect;
	}
	m_coverageHighlights.clear();

	// 绘制目标位置高亮
	for (int dy = 0; dy < gridH; ++dy)
	{
		for (int dx = 0; dx < gridW; ++dx)
		{
			int gx = gridPos.x() + dx;
			int gy = gridPos.y() + dy;

			if (gx >= m_mapWidth || gy >= m_mapHeight)
				continue;

			QPointF topLeft = gridToScene(gx, gy);
			auto* rect = new QGraphicsRectItem(topLeft.x(), topLeft.y(), m_tileWidth, m_tileHeight);

			if (dx == 0 && dy == 0)
			{
				rect->setPen(QPen(QColor(100, 200, 100), 2));
				rect->setBrush(QBrush(QColor(100, 200, 100, 60)));
			}
			else
			{
				rect->setPen(QPen(QColor(100, 200, 100), 1));
				rect->setBrush(QBrush(QColor(100, 200, 100, 40)));
			}

			rect->setZValue(999);
			m_scene->addItem(rect);
			m_coverageHighlights.append(rect);
		}
	}
}

// ============== 复制瓦片 ==============

bool MapViewWidget::hasPlacedTileAt(int gridX, int gridY, int layer) const
{
	for (const auto* tile : m_placedTiles)
	{
		if (tile->layer() != layer)
			continue;

		// 检查瓦片覆盖的所有网格
		int tx = tile->gridX();
		int ty = tile->gridY();
		int tw = tile->gridWidth();
		int th = tile->gridHeight();

		if (gridX >= tx && gridX < tx + tw && gridY >= ty && gridY < ty + th)
			return true;
	}
	return false;
}

MapTileItem* MapViewWidget::copyTileToGrid(MapTileItem* sourceTile, int gridX, int gridY)
{
	if (!sourceTile)
		return nullptr;

	// 检查边界
	int gridW = sourceTile->gridWidth();
	int gridH = sourceTile->gridHeight();

	if (gridX < 0 || gridY < 0 ||
		gridX + gridW > m_mapWidth || gridY + gridH > m_mapHeight)
	{
		return nullptr;
	}

	// 检查该位置是否已有瓦片
	if (hasPlacedTileAt(gridX, gridY, m_currentLayer))
	{
		return nullptr;
	}

	// 创建新瓦片
	auto* newTile = new MapTileItem(
		sourceTile->originalPixmap(),
		sourceTile->slice(),
		gridX,
		gridY,
		m_currentLayer
	);

	newTile->setTilesetId(sourceTile->tilesetId());
	newTile->setGridSize(gridW, gridH);
	newTile->setPos(gridToScene(gridX, gridY));
	newTile->setZValue(10 + m_currentLayer);

	// 复制可编辑属性
	newTile->setCollisionType(sourceTile->collisionType());
	newTile->setTags(sourceTile->tags());
	newTile->setDisplayName(sourceTile->displayName());

	// 连接信号
	connect(newTile, &MapTileItem::clicked, this, &MapViewWidget::onTileClicked);
	connect(newTile, &MapTileItem::dragStarted, this, &MapViewWidget::onTileDragStarted);
	connect(newTile, &MapTileItem::dragFinished, this, &MapViewWidget::onTileDragFinished);
	connect(newTile, &MapTileItem::copyDragStarted, this, &MapViewWidget::onCopyDragStarted);
	connect(newTile, &MapTileItem::copyDragMoved, this, &MapViewWidget::onCopyDragMoved);
	connect(newTile, &MapTileItem::copyDragFinished, this, &MapViewWidget::onCopyDragFinished);
	connect(newTile, &MapTileItem::deleteDragStarted, this, &MapViewWidget::onDeleteDragStarted);
	connect(newTile, &MapTileItem::deleteDragMoved, this, &MapViewWidget::onDeleteDragMoved);
	connect(newTile, &MapTileItem::deleteDragFinished, this, &MapViewWidget::onDeleteDragFinished);

	m_scene->addItem(newTile);
	m_placedTiles.append(newTile);

	qDebug() << "Copied tile to grid:" << gridX << "," << gridY;

	return newTile;
}

void MapViewWidget::onCopyDragStarted(MapTileItem* tile, CornerZone corner)
{
	if (!tile || tile != m_selectedTile)
		return;

	m_copyDragging = true;
	m_copyStartGrid = QPoint(tile->gridX(), tile->gridY());
	m_copyLastGrid = m_copyStartGrid;
	m_copyPlacedPositions.clear();

	// 记录源瓦片位置（不重复放置）
	m_copyPlacedPositions.insert(qMakePair(tile->gridX(), tile->gridY()));

	qDebug() << "Copy drag started from corner:" << static_cast<int>(corner)
		<< "at grid:" << m_copyStartGrid;
}

void MapViewWidget::onCopyDragMoved(MapTileItem* tile, const QPointF& scenePos)
{
	if (!m_copyDragging || !tile)
		return;

	QPoint currentGrid = sceneToGrid(scenePos);

	// 如果网格位置没变，不处理
	if (currentGrid == m_copyLastGrid)
		return;

	m_copyLastGrid = currentGrid;

	// 计算需要填充的矩形区域
	int startX = qMin(m_copyStartGrid.x(), currentGrid.x());
	int startY = qMin(m_copyStartGrid.y(), currentGrid.y());
	int endX = qMax(m_copyStartGrid.x(), currentGrid.x());
	int endY = qMax(m_copyStartGrid.y(), currentGrid.y());

	// 更新高亮显示
	updateCopyHighlight(QPoint(startX, startY), QPoint(endX, endY));

	// 动态放置瓦片：遍历矩形区域，放置还没有放置过的位置
	int gridW = tile->gridWidth();
	int gridH = tile->gridHeight();

	for (int gy = startY; gy <= endY; gy += gridH)
	{
		for (int gx = startX; gx <= endX; gx += gridW)
		{
			QPair<int, int> pos = qMakePair(gx, gy);

			// 跳过已经放置过的位置
			if (m_copyPlacedPositions.contains(pos))
				continue;

			// 检查边界
			if (gx < 0 || gy < 0 || gx + gridW > m_mapWidth || gy + gridH > m_mapHeight)
				continue;

			// 尝试放置
			MapTileItem* newTile = copyTileToGrid(tile, gx, gy);
			if (newTile)
			{
				m_copyPlacedPositions.insert(pos);
			}
		}
	}
}

void MapViewWidget::onCopyDragFinished(MapTileItem* tile)
{
	Q_UNUSED(tile);

	m_copyDragging = false;
	m_copyPlacedPositions.clear();
	clearCopyHighlight();

	qDebug() << "Copy drag finished";
}

void MapViewWidget::updateCopyHighlight(const QPoint& startGrid, const QPoint& endGrid)
{
	clearCopyHighlight();

	// 绘制整个复制区域的高亮
	for (int gy = startGrid.y(); gy <= endGrid.y(); ++gy)
	{
		for (int gx = startGrid.x(); gx <= endGrid.x(); ++gx)
		{
			if (gx < 0 || gy < 0 || gx >= m_mapWidth || gy >= m_mapHeight)
				continue;

			QPointF topLeft = gridToScene(gx, gy);
			auto* rect = new QGraphicsRectItem(topLeft.x(), topLeft.y(), m_tileWidth, m_tileHeight);

			// 使用不同颜色区分已放置和待放置
			if (hasPlacedTileAt(gx, gy, m_currentLayer))
			{
				rect->setPen(QPen(QColor(200, 200, 200), 1));
				rect->setBrush(QBrush(QColor(200, 200, 200, 30)));
			}
			else
			{
				rect->setPen(QPen(QColor(100, 200, 100), 1));
				rect->setBrush(QBrush(QColor(100, 200, 100, 40)));
			}

			rect->setZValue(998);
			m_scene->addItem(rect);
			m_copyHighlights.append(rect);
		}
	}
}

void MapViewWidget::clearCopyHighlight()
{
	for (auto* rect : m_copyHighlights)
	{
		m_scene->removeItem(rect);
		delete rect;
	}
	m_copyHighlights.clear();
}

// ============== 删除瓦片 ==============

MapTileItem* MapViewWidget::getTileAtGrid(int gridX, int gridY, int layer) const
{
	for (auto* tile : m_placedTiles)
	{
		if (tile->layer() != layer)
			continue;

		int tx = tile->gridX();
		int ty = tile->gridY();
		int tw = tile->gridWidth();
		int th = tile->gridHeight();

		if (gridX >= tx && gridX < tx + tw && gridY >= ty && gridY < ty + th)
			return tile;
	}
	return nullptr;
}

void MapViewWidget::deleteTileAtGrid(int gridX, int gridY)
{
	MapTileItem* tile = getTileAtGrid(gridX, gridY, m_currentLayer);
	if (!tile)
		return;

	// 如果是当前选中的瓦片，不删除（保留源瓦片）
	if (tile == m_selectedTile)
		return;

	// 从列表中移除
	m_placedTiles.removeOne(tile);

	// 从场景中移除
	m_scene->removeItem(tile);

	// 删除对象
	tile->deleteLater();

	qDebug() << "Deleted tile at grid:" << gridX << "," << gridY;
}

void MapViewWidget::onDeleteDragStarted(MapTileItem* tile, CornerZone corner)
{
	if (!tile || tile != m_selectedTile)
		return;

	m_deleteDragging = true;
	m_deleteStartGrid = QPoint(tile->gridX(), tile->gridY());
	m_deleteLastGrid = m_deleteStartGrid;
	m_deleteRemovedPositions.clear();

	// 记录源瓦片位置（不删除源瓦片）
	m_deleteRemovedPositions.insert(qMakePair(tile->gridX(), tile->gridY()));

	qDebug() << "Delete drag started from corner:" << static_cast<int>(corner)
		<< "at grid:" << m_deleteStartGrid;
}

void MapViewWidget::onDeleteDragMoved(MapTileItem* tile, const QPointF& scenePos)
{
	if (!m_deleteDragging || !tile)
		return;

	QPoint currentGrid = sceneToGrid(scenePos);

	// 如果网格位置没变，不处理
	if (currentGrid == m_deleteLastGrid)
		return;

	m_deleteLastGrid = currentGrid;

	// 计算需要删除的矩形区域
	int startX = qMin(m_deleteStartGrid.x(), currentGrid.x());
	int startY = qMin(m_deleteStartGrid.y(), currentGrid.y());
	int endX = qMax(m_deleteStartGrid.x(), currentGrid.x());
	int endY = qMax(m_deleteStartGrid.y(), currentGrid.y());

	// 更新高亮显示
	updateDeleteHighlight(QPoint(startX, startY), QPoint(endX, endY));

	// 动态删除瓦片：遍历矩形区域
	for (int gy = startY; gy <= endY; ++gy)
	{
		for (int gx = startX; gx <= endX; ++gx)
		{
			QPair<int, int> pos = qMakePair(gx, gy);

			// 跳过已经处理过的位置
			if (m_deleteRemovedPositions.contains(pos))
				continue;

			// 检查边界
			if (gx < 0 || gy < 0 || gx >= m_mapWidth || gy >= m_mapHeight)
				continue;

			// 尝试删除
			deleteTileAtGrid(gx, gy);
			m_deleteRemovedPositions.insert(pos);
		}
	}
}

void MapViewWidget::onDeleteDragFinished(MapTileItem* tile)
{
	Q_UNUSED(tile);

	m_deleteDragging = false;
	m_deleteRemovedPositions.clear();
	clearDeleteHighlight();

	qDebug() << "Delete drag finished";
}

void MapViewWidget::updateDeleteHighlight(const QPoint& startGrid, const QPoint& endGrid)
{
	clearDeleteHighlight();

	// 绘制整个删除区域的高亮（红色）
	for (int gy = startGrid.y(); gy <= endGrid.y(); ++gy)
	{
		for (int gx = startGrid.x(); gx <= endGrid.x(); ++gx)
		{
			if (gx < 0 || gy < 0 || gx >= m_mapWidth || gy >= m_mapHeight)
				continue;

			QPointF topLeft = gridToScene(gx, gy);
			auto* rect = new QGraphicsRectItem(topLeft.x(), topLeft.y(), m_tileWidth, m_tileHeight);

			// 检查是否是源瓦片位置
			bool isSourceTile = (gx == m_deleteStartGrid.x() && gy == m_deleteStartGrid.y());

			if (isSourceTile)
			{
				// 源瓦片位置用蓝色标记（不会被删除）
				rect->setPen(QPen(QColor(80, 140, 255), 2));
				rect->setBrush(QBrush(QColor(80, 140, 255, 40)));
			}
			else if (hasPlacedTileAt(gx, gy, m_currentLayer))
			{
				// 有瓦片的位置用红色标记（将被删除）
				rect->setPen(QPen(QColor(255, 100, 100), 2));
				rect->setBrush(QBrush(QColor(255, 100, 100, 60)));
			}
			else
			{
				// 空位置用浅红色标记
				rect->setPen(QPen(QColor(255, 150, 150), 1));
				rect->setBrush(QBrush(QColor(255, 150, 150, 30)));
			}

			rect->setZValue(998);
			m_scene->addItem(rect);
			m_deleteHighlights.append(rect);
		}
	}
}

void MapViewWidget::clearDeleteHighlight()
{
	for (auto* rect : m_deleteHighlights)
	{
		m_scene->removeItem(rect);
		delete rect;
	}
	m_deleteHighlights.clear();
}

// ============== 拖放事件 ==============

void MapViewWidget::dragEnterEvent(QDragEnterEvent* event)
{
	if (event->mimeData()->hasFormat(TILE_MIME_TYPE))
	{
		auto* tileMime = qobject_cast<const TileMimeData*>(event->mimeData());
		if (tileMime && tileMime->tileData().isValid())
		{
			// 验证尺寸
			const auto& slice = tileMime->tileData().slice;
			if (validateTileSize(slice.width, slice.height))
			{
				event->acceptProposedAction();
				return;
			}
		}
	}
	event->ignore();
}

void MapViewWidget::dragMoveEvent(QDragMoveEvent* event)
{
	if (!event->mimeData()->hasFormat(TILE_MIME_TYPE))
	{
		event->ignore();
		return;
	}

	auto* tileMime = qobject_cast<const TileMimeData*>(event->mimeData());
	if (!tileMime)
	{
		event->ignore();
		return;
	}

	// 验证尺寸
	const auto& slice = tileMime->tileData().slice;
	if (!validateTileSize(slice.width, slice.height))
	{
		clearDropHighlight();
		event->ignore();
		return;
	}

	QPointF scenePos = mapToScene(event->position().toPoint());
	updateDropHighlight(scenePos, tileMime->tileData());

	event->acceptProposedAction();
}

void MapViewWidget::dragLeaveEvent(QDragLeaveEvent* event)
{
	Q_UNUSED(event);
	clearDropHighlight();
}

void MapViewWidget::dropEvent(QDropEvent* event)
{
	if (!event->mimeData()->hasFormat(TILE_MIME_TYPE))
	{
		event->ignore();
		return;
	}

	auto* tileMime = qobject_cast<const TileMimeData*>(event->mimeData());
	if (!tileMime || !tileMime->tileData().isValid())
	{
		event->ignore();
		return;
	}

	// 验证尺寸
	const auto& slice = tileMime->tileData().slice;
	if (!validateTileSize(slice.width, slice.height))
	{
		clearDropHighlight();
		event->ignore();
		qDebug() << "Drop rejected: tile size" << slice.width << "x" << slice.height
			<< "doesn't match grid size" << m_tileWidth << "x" << m_tileHeight;
		return;
	}

	QPointF scenePos = mapToScene(event->position().toPoint());
	QPoint gridPos = sceneToGrid(scenePos);

	if (gridPos.x() >= 0 && gridPos.y() >= 0 &&
		gridPos.x() < m_mapWidth && gridPos.y() < m_mapHeight)
	{
		placeTile(tileMime->tileData(), gridPos);
		event->acceptProposedAction();
	}
	else
	{
		event->ignore();
	}

	clearDropHighlight();
}

void MapViewWidget::updateDropHighlight(const QPointF& scenePos, const TileDragData& tileData)
{
	QPoint gridPos = sceneToGrid(scenePos);

	if (gridPos.x() < 0 || gridPos.y() < 0 ||
		gridPos.x() >= m_mapWidth || gridPos.y() >= m_mapHeight)
	{
		clearDropHighlight();
		return;
	}

	int gridW = tileData.slice.width / m_tileWidth;
	int gridH = tileData.slice.height / m_tileHeight;

	for (auto* rect : m_coverageHighlights)
	{
		m_scene->removeItem(rect);
		delete rect;
	}
	m_coverageHighlights.clear();

	for (int dy = 0; dy < gridH; ++dy)
	{
		for (int dx = 0; dx < gridW; ++dx)
		{
			int gx = gridPos.x() + dx;
			int gy = gridPos.y() + dy;

			if (gx >= m_mapWidth || gy >= m_mapHeight)
				continue;

			QPointF topLeft = gridToScene(gx, gy);
			auto* rect = new QGraphicsRectItem(topLeft.x(), topLeft.y(), m_tileWidth, m_tileHeight);

			if (dx == 0 && dy == 0)
			{
				rect->setPen(QPen(QColor(80, 140, 255), 2));
				rect->setBrush(QBrush(QColor(80, 140, 255, 60)));
			}
			else
			{
				rect->setPen(QPen(QColor(255, 180, 80), 1));
				rect->setBrush(QBrush(QColor(255, 180, 80, 40)));
			}

			rect->setZValue(999);
			m_scene->addItem(rect);
			m_coverageHighlights.append(rect);
		}
	}

	QPointF topLeft = gridToScene(gridPos.x(), gridPos.y());
	m_dropHighlight->setRect(topLeft.x(), topLeft.y(),
		gridW * m_tileWidth, gridH * m_tileHeight);
	m_dropHighlight->setVisible(false);
}

void MapViewWidget::clearDropHighlight()
{
	m_dropHighlight->setVisible(false);

	for (auto* rect : m_coverageHighlights)
	{
		m_scene->removeItem(rect);
		delete rect;
	}
	m_coverageHighlights.clear();
}

QPoint MapViewWidget::sceneToGrid(const QPointF& scenePos) const
{
	int gridX = static_cast<int>(qFloor(scenePos.x() / m_tileWidth));
	int gridY = static_cast<int>(qFloor(scenePos.y() / m_tileHeight));
	return QPoint(gridX, gridY);
}

QPointF MapViewWidget::gridToScene(int gridX, int gridY) const
{
	return QPointF(gridX * m_tileWidth, gridY * m_tileHeight);
}

void MapViewWidget::placeTile(const TileDragData& tileData, const QPoint& gridPos)
{
	if (!tileData.isValid())
		return;

	// 因为已经验证过，直接用整除
	int gridW = tileData.slice.width / m_tileWidth;
	int gridH = tileData.slice.height / m_tileHeight;

	QPixmap scaledPixmap = tileData.pixmap.scaled(
		gridW * m_tileWidth,
		gridH * m_tileHeight,
		Qt::IgnoreAspectRatio,
		Qt::SmoothTransformation
	);

	// 创建瓦片图元，传入当前图层
	auto* tileItem = new MapTileItem(scaledPixmap, tileData.slice, gridPos.x(), gridPos.y(), m_currentLayer);
	tileItem->setTilesetId(tileData.tilesetId);
	tileItem->setGridSize(gridW, gridH);
	tileItem->setPos(gridToScene(gridPos.x(), gridPos.y()));
	tileItem->setZValue(10 + m_currentLayer);

	// 连接点击和拖动信号
	connect(tileItem, &MapTileItem::clicked, this, &MapViewWidget::onTileClicked);
	connect(tileItem, &MapTileItem::dragStarted, this, &MapViewWidget::onTileDragStarted);
	connect(tileItem, &MapTileItem::dragFinished, this, &MapViewWidget::onTileDragFinished);
	connect(tileItem, &MapTileItem::copyDragStarted, this, &MapViewWidget::onCopyDragStarted);
	connect(tileItem, &MapTileItem::copyDragMoved, this, &MapViewWidget::onCopyDragMoved);
	connect(tileItem, &MapTileItem::copyDragFinished, this, &MapViewWidget::onCopyDragFinished);
	connect(tileItem, &MapTileItem::deleteDragStarted, this, &MapViewWidget::onDeleteDragStarted);
	connect(tileItem, &MapTileItem::deleteDragMoved, this, &MapViewWidget::onDeleteDragMoved);
	connect(tileItem, &MapTileItem::deleteDragFinished, this, &MapViewWidget::onDeleteDragFinished);

	m_scene->addItem(tileItem);
	m_placedTiles.append(tileItem);

	qDebug() << "Placed tile:" << tileData.slice.name
		<< "at grid:" << gridPos
		<< "size:" << gridW << "x" << gridH
		<< "layer:" << m_currentLayer;
}

// ============== 键盘和鼠标事件 ==============

void MapViewWidget::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Space && !event->isAutoRepeat())
	{
		m_spacePressed = true;
		if (!m_panning)
			setCursor(Qt::OpenHandCursor);
		return;
	}

	// Delete 键删除选中的瓦片
	if (event->key() == Qt::Key_Delete && m_selectedTile)
	{
		deleteSelectedTile();
		return;
	}

	// Escape 键取消选中
	if (event->key() == Qt::Key_Escape && m_selectedTile)
	{
		clearSelection();
		return;
	}

	// H 键水平翻转选中的瓦片
	if (event->key() == Qt::Key_H && m_selectedTile)
	{
		m_selectedTile->toggleFlipX();
		return;
	}

	// V 键垂直翻转选中的瓦片
	if (event->key() == Qt::Key_V && m_selectedTile)
	{
		m_selectedTile->toggleFlipY();
		return;
	}

	QGraphicsView::keyPressEvent(event);
}

void MapViewWidget::keyReleaseEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Space && !event->isAutoRepeat())
	{
		m_spacePressed = false;
		if (!m_panning)
			unsetCursor();
		return;
	}
	QGraphicsView::keyReleaseEvent(event);
}

void MapViewWidget::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton && m_spacePressed)
	{
		m_panning = true;
		m_lastMousePos = event->pos();
		setCursor(Qt::ClosedHandCursor);
		return;
	}

	// 点击空白区域取消选中
	if (event->button() == Qt::LeftButton && !m_spacePressed)
	{
		QPointF scenePos = mapToScene(event->pos());
		QGraphicsItem* item = m_scene->itemAt(scenePos, transform());

		// 如果点击的不是 MapTileItem，或者是不在当前图层的瓦片，则取消选中
		MapTileItem* tileItem = dynamic_cast<MapTileItem*>(item);
		if (!tileItem || tileItem->layer() != m_currentLayer)
		{
			clearSelection();
		}
	}

	QGraphicsView::mousePressEvent(event);
}

void MapViewWidget::mouseMoveEvent(QMouseEvent* event)
{
	if (m_panning)
	{
		QPoint delta = event->pos() - m_lastMousePos;
		m_lastMousePos = event->pos();

		horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
		verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
		return;
	}

	// 如果有瓦片在拖动，更新高亮
	if (m_tileDragging && m_selectedTile)
	{
		QPointF scenePos = mapToScene(event->pos());
		updateMoveHighlight(scenePos, m_selectedTile);
	}

	QGraphicsView::mouseMoveEvent(event);
}

void MapViewWidget::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton && m_panning)
	{
		m_panning = false;
		if (m_spacePressed)
			setCursor(Qt::OpenHandCursor);
		else
			unsetCursor();
		return;
	}
	QGraphicsView::mouseReleaseEvent(event);
}

void MapViewWidget::wheelEvent(QWheelEvent* event)
{
	const double scaleFactor = 1.15;
	if (event->angleDelta().y() > 0)
	{
		applyZoom(scaleFactor);
	}
	else
	{
		applyZoom(1.0 / scaleFactor);
	}
}

void MapViewWidget::clearAllTiles()
{
	// 先清除选中
	clearSelection();

	// 删除所有瓦片
	for (auto* tile : m_placedTiles)
	{
		m_scene->removeItem(tile);
		delete tile;
	}
	m_placedTiles.clear();

	qDebug() << "Cleared all tiles";
}

// ============== 导入时放置瓦片 ==============
void MapViewWidget::placeTileAt(int gridX, int gridY, const QString& tilesetId, const SpriteSlice& slice,
	const QPixmap& pixmap, int layer, const QString& displayName,
	CollisionType collisionType, const QString& tags,
	bool flipX, bool flipY)
{
	// 检查边界
	int gridW = slice.width / m_tileWidth;
	int gridH = slice.height / m_tileHeight;

	if (gridW <= 0) gridW = 1;
	if (gridH <= 0) gridH = 1;

	if (gridX < 0 || gridY < 0 ||
		gridX + gridW > m_mapWidth || gridY + gridH > m_mapHeight)
	{
		qWarning() << "placeTileAt: out of bounds - grid:" << gridX << "," << gridY
			<< "size:" << gridW << "x" << gridH
			<< "map:" << m_mapWidth << "x" << m_mapHeight;
		return;
	}

	// 缩放 pixmap 以匹配网格尺寸
	QPixmap scaledPixmap = pixmap.scaled(
		gridW * m_tileWidth,
		gridH * m_tileHeight,
		Qt::IgnoreAspectRatio,
		Qt::SmoothTransformation
	);

	// 创建瓦片图元
	auto* tileItem = new MapTileItem(scaledPixmap, slice, gridX, gridY, layer);
	tileItem->setTilesetId(tilesetId);
	tileItem->setGridSize(gridW, gridH);
	tileItem->setPos(gridToScene(gridX, gridY));
	tileItem->setZValue(10 + layer);

	// 设置额外属性
	if (!displayName.isEmpty())
	{
		tileItem->setDisplayName(displayName);
	}
	tileItem->setCollisionType(collisionType);
	tileItem->setTags(tags);

	// 设置翻转状态
	if (flipX)
	{
		tileItem->setFlipX(true);
	}
	if (flipY)
	{
		tileItem->setFlipY(true);
	}

	// 连接信号
	connect(tileItem, &MapTileItem::clicked, this, &MapViewWidget::onTileClicked);
	connect(tileItem, &MapTileItem::dragStarted, this, &MapViewWidget::onTileDragStarted);
	connect(tileItem, &MapTileItem::dragFinished, this, &MapViewWidget::onTileDragFinished);
	connect(tileItem, &MapTileItem::copyDragStarted, this, &MapViewWidget::onCopyDragStarted);
	connect(tileItem, &MapTileItem::copyDragMoved, this, &MapViewWidget::onCopyDragMoved);
	connect(tileItem, &MapTileItem::copyDragFinished, this, &MapViewWidget::onCopyDragFinished);
	connect(tileItem, &MapTileItem::deleteDragStarted, this, &MapViewWidget::onDeleteDragStarted);
	connect(tileItem, &MapTileItem::deleteDragMoved, this, &MapViewWidget::onDeleteDragMoved);
	connect(tileItem, &MapTileItem::deleteDragFinished, this, &MapViewWidget::onDeleteDragFinished);

	m_scene->addItem(tileItem);
	m_placedTiles.append(tileItem);

	qDebug() << "Imported tile:" << slice.name
		<< "at grid:" << gridX << "," << gridY
		<< "size:" << gridW << "x" << gridH
		<< "layer:" << layer
		<< "flipX:" << flipX << "flipY:" << flipY;
}
