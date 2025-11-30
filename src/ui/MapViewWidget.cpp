// src/ui/MapViewWidget.cpp
#include "MapViewWidget.h"
#include "app/AppContext.h"
#include "app/DocumentManager.h"
#include "core/MapDocument.h"

#include <QPainter>
#include <QPaintEvent>

MapViewWidget::MapViewWidget(QWidget* parent)
	: QWidget(parent), m_ctx(nullptr)
{
	setAutoFillBackground(false);
	setMouseTracking(true);
	setFocusPolicy(Qt::ClickFocus);   // 点击后能接收空格按键
}

void MapViewWidget::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);

	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing, false);

	p.fillRect(rect(), QColor(245, 246, 248));

	const MapDocument* doc = m_ctx->documentManager.document();
	if (!doc) {
		p.setPen(QColor(150, 150, 150));
		p.drawText(rect(), Qt::AlignCenter, QStringLiteral("No map loaded"));
		return;
	}

	const int tileW = doc->tileWidth;
	const int tileH = doc->tileHeight;

	QRect gridArea = rect().adjusted(kMargin, kMargin, -kMargin, -kMargin);
	if (gridArea.width() <= 0 || gridArea.height() <= 0)
		return;

	// ---- 画网格外框（可选）----
	p.setPen(QColor(190, 190, 195));
	p.drawRect(gridArea);

	// ---- 画平移网格线 ----
	p.setPen(QColor(220, 220, 225));

	const int originX = gridArea.left() + m_viewOffset.x();
	const int originY = gridArea.top() + m_viewOffset.y();

	// 垂直线
	int firstCol = static_cast<int>(std::floor((gridArea.left() - originX) / double(tileW)));
	int lastCol = static_cast<int>(std::floor((gridArea.right() - originX) / double(tileW)));
	for (int col = firstCol; col <= lastCol; ++col) {
		int x = originX + col * tileW;
		p.drawLine(x, gridArea.top(), x, gridArea.bottom());
	}

	// 水平线
	int firstRow = static_cast<int>(std::floor((gridArea.top() - originY) / double(tileH)));
	int lastRow = static_cast<int>(std::floor((gridArea.bottom() - originY) / double(tileH)));
	for (int row = firstRow; row <= lastRow; ++row) {
		int y = originY + row * tileH;
		p.drawLine(gridArea.left(), y, gridArea.right(), y);
	}

	// ---- 选中格子及四周高亮 ----
	if (m_selectedTile.x() >= 0 && m_selectedTile.y() >= 0) {
		auto drawCell = [&](const QPoint& tile,
			const QColor& fill,
			const QColor& border,
			int borderWidth)
			{
				if (tile.x() < 0 || tile.y() < 0 ||
					tile.x() >= doc->width || tile.y() >= doc->height)
					return;

				QRect r = TileRect(tile, doc, gridArea);
				if (!r.intersects(gridArea))
					return;

				p.save();
				p.setPen(QPen(border, borderWidth));
				p.setBrush(fill);
				p.drawRect(r.adjusted(1, 1, -1, -1));
				p.restore();
			};

		// 中心格子：蓝边 + 轻微填充
		drawCell(m_selectedTile,
			QColor(80, 140, 255, 40),
			QColor(80, 140, 255),
			2);

		// 四邻格子：橙色半透明填充
		static const QPoint offsets[4] = {
			{ 1,  0},
			{-1,  0},
			{ 0,  1},
			{ 0, -1}
		};
		for (const QPoint& off : offsets) {
			drawCell(m_selectedTile + off,
				QColor(255, 180, 80, 40),
				QColor(255, 180, 80, 150),
				1);
		}
	}

	// TODO: 将来在这里再叠加 tile 美术 / 选择框 / 拖拽预览等
}

void MapViewWidget::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
		m_spacePressed = true;
		if (!m_panning)
			setCursor(Qt::OpenHandCursor);
		return;
	}

	QWidget::keyPressEvent(event);
}

void MapViewWidget::keyReleaseEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
		m_spacePressed = false;
		if (!m_panning)
			unsetCursor();   // 恢复默认光标
		return;
	}

	QWidget::keyReleaseEvent(event);
}

void MapViewWidget::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton && m_spacePressed) {
		// 进入拖动模式
		m_panning = true;
		m_lastMousePos = event->pos();
		setCursor(Qt::ClosedHandCursor);
		return;
	}

	if (event->button() == Qt::LeftButton && !m_spacePressed) {
		QPoint tile = TileFromPos(event->pos());
		if (tile.x() >= 0) {
			m_selectedTile = tile;
			update();           // 触发重绘，高亮更新
		}
		return;
	}

	QWidget::mousePressEvent(event);
}

void MapViewWidget::mouseMoveEvent(QMouseEvent* event)
{
	if (m_panning) {
		QPoint delta = event->pos() - m_lastMousePos;
		m_lastMousePos = event->pos();

		m_viewOffset += delta;   // 更新视图偏移
		update();                // 触发重绘
		return;
	}

	QWidget::mouseMoveEvent(event);
}

void MapViewWidget::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton && m_panning) {
		m_panning = false;
		if (m_spacePressed)
			setCursor(Qt::OpenHandCursor);
		else
			unsetCursor();
		return;
	}

	QWidget::mouseReleaseEvent(event);
}

QPoint MapViewWidget::TileFromPos(const QPoint& pos) const
{
	const MapDocument* doc = m_ctx->documentManager.document();
	if (!doc)
		return QPoint(-1, -1);

	const int tileW = doc->tileWidth;
	const int tileH = doc->tileHeight;

	QRect gridArea = rect().adjusted(kMargin, kMargin, -kMargin, -kMargin);
	if (!gridArea.contains(pos))
		return QPoint(-1, -1);

	const int originX = gridArea.left() + m_viewOffset.x();
	const int originY = gridArea.top() + m_viewOffset.y();

	const double localX = pos.x() - originX;
	const double localY = pos.y() - originY;

	int col = static_cast<int>(std::floor(localX / tileW));
	int row = static_cast<int>(std::floor(localY / tileH));

	if (col < 0 || row < 0 || col >= doc->width || row >= doc->height)
		return QPoint(-1, -1);

	return QPoint(col, row);
}

QRect MapViewWidget::TileRect(const QPoint& tile,
	const MapDocument* doc,
	const QRect& gridArea) const
{
	const int tileW = doc->tileWidth;
	const int tileH = doc->tileHeight;

	const int originX = gridArea.left() + m_viewOffset.x();
	const int originY = gridArea.top() + m_viewOffset.y();

	int x = originX + tile.x() * tileW;
	int y = originY + tile.y() * tileH;

	return QRect(x, y, tileW, tileH);
}

