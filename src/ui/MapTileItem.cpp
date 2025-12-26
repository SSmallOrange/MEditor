#include "MapTileItem.h"
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QStyleOptionGraphicsItem>
#include <QApplication>
#include <QCursor>

MapTileItem::MapTileItem(const QPixmap& pixmap, const SpriteSlice& slice,
	int gridX, int gridY, int layer, QGraphicsItem* parent)
	: QGraphicsPixmapItem(pixmap, parent)
	, m_slice(slice)
	, m_gridX(gridX)
	, m_gridY(gridY)
	, m_layer(layer)
	, m_collisionType(slice.collisionType)
	, m_tags(slice.tags)
	, m_displayName(slice.name)
	, m_originalPixmap(pixmap)  // 保存原始 pixmap
{
	setTransformationMode(Qt::SmoothTransformation);
	setAcceptedMouseButtons(Qt::LeftButton);
	setFlag(QGraphicsItem::ItemIsSelectable, false);
	setAcceptHoverEvents(true);  // 启用悬停事件
}

void MapTileItem::setGridPos(int x, int y)
{
	m_gridX = x;
	m_gridY = y;
}

void MapTileItem::setSelected(bool selected)
{
	if (m_selected == selected)
		return;

	m_selected = selected;
	update();
	emit selectionChanged(this, selected);
}

CornerZone MapTileItem::detectCornerZone(const QPointF& localPos) const
{
	if (!m_selected)
		return CornerZone::None;

	QRectF rect = boundingRect();
	qreal hitSize = CORNER_HIT_SIZE;

	// 左上角
	if (localPos.x() <= hitSize && localPos.y() <= hitSize)
		return CornerZone::TopLeft;

	// 右上角
	if (localPos.x() >= rect.width() - hitSize && localPos.y() <= hitSize)
		return CornerZone::TopRight;

	// 左下角
	if (localPos.x() <= hitSize && localPos.y() >= rect.height() - hitSize)
		return CornerZone::BottomLeft;

	// 右下角
	if (localPos.x() >= rect.width() - hitSize && localPos.y() >= rect.height() - hitSize)
		return CornerZone::BottomRight;

	return CornerZone::None;
}

void MapTileItem::updateCursorForZone(CornerZone zone, bool shiftPressed)
{
	if (zone == CornerZone::None)
	{
		unsetCursor();
		return;
	}

	// Shift 按下时显示禁止光标（表示删除模式）
	if (shiftPressed)
	{
		setCursor(Qt::ForbiddenCursor);
	}
	else
	{
		// 普通模式显示对角箭头
		switch (zone)
		{
		case CornerZone::TopLeft:
		case CornerZone::BottomRight:
			setCursor(Qt::SizeFDiagCursor);
			break;
		case CornerZone::TopRight:
		case CornerZone::BottomLeft:
			setCursor(Qt::SizeBDiagCursor);
			break;
		default:
			unsetCursor();
			break;
		}
	}
}

void MapTileItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
	if (m_selected)
	{
		CornerZone zone = detectCornerZone(event->pos());
		m_currentCornerZone = zone;
		bool shiftPressed = event->modifiers() & Qt::ShiftModifier;
		updateCursorForZone(zone, shiftPressed);
	}
	QGraphicsPixmapItem::hoverEnterEvent(event);
}

void MapTileItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
	if (m_selected)
	{
		CornerZone zone = detectCornerZone(event->pos());
		bool shiftPressed = event->modifiers() & Qt::ShiftModifier;

		if (zone != m_currentCornerZone)
		{
			m_currentCornerZone = zone;
			update();  // 更新绘制以显示角落高亮
		}
		updateCursorForZone(zone, shiftPressed);
	}
	QGraphicsPixmapItem::hoverMoveEvent(event);
}

void MapTileItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
	m_currentCornerZone = CornerZone::None;
	unsetCursor();
	update();
	QGraphicsPixmapItem::hoverLeaveEvent(event);
}

void MapTileItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		m_dragStartPos = event->scenePos();
		m_originalPos = pos();

		// 检查是否在角落区域
		CornerZone zone = detectCornerZone(event->pos());
		if (m_selected && zone != CornerZone::None)
		{
			bool shiftPressed = event->modifiers() & Qt::ShiftModifier;

			if (shiftPressed)
			{
				// Shift + 角落拖动 = 删除模式
				m_deleteDragging = true;
				m_deleteStartCorner = zone;
				emit deleteDragStarted(this, zone);
			}
			else
			{
				// 普通角落拖动 = 复制模式
				m_copyDragging = true;
				m_copyStartCorner = zone;
				emit copyDragStarted(this, zone);
			}
			event->accept();
			return;
		}

		emit clicked(this);
		event->accept();
		return;
	}
	QGraphicsPixmapItem::mousePressEvent(event);
}

void MapTileItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
	// 删除拖动
	if (m_deleteDragging)
	{
		emit deleteDragMoved(this, event->scenePos());
		event->accept();
		return;
	}

	// 角落复制拖动
	if (m_copyDragging)
	{
		emit copyDragMoved(this, event->scenePos());
		event->accept();
		return;
	}

	if (!m_selected)
	{
		QGraphicsPixmapItem::mouseMoveEvent(event);
		return;
	}

	if (event->buttons() & Qt::LeftButton)
	{
		QPointF delta = event->scenePos() - m_dragStartPos;

		// 检查是否超过拖动阈值
		if (!m_dragging && delta.manhattanLength() >= QApplication::startDragDistance())
		{
			m_dragging = true;
			emit dragStarted(this);
			setOpacity(0.7);  // 拖动时半透明
		}

		if (m_dragging)
		{
			setPos(m_originalPos + delta);
		}
	}

	event->accept();
}

void MapTileItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		// 删除拖动结束
		if (m_deleteDragging)
		{
			m_deleteDragging = false;
			m_deleteStartCorner = CornerZone::None;
			emit deleteDragFinished(this);
			event->accept();
			return;
		}

		// 角落复制拖动结束
		if (m_copyDragging)
		{
			m_copyDragging = false;
			m_copyStartCorner = CornerZone::None;
			emit copyDragFinished(this);
			event->accept();
			return;
		}

		// 普通拖动结束
		if (m_dragging)
		{
			m_dragging = false;
			setOpacity(1.0);  // 恢复不透明
			emit dragFinished(this, event->scenePos());
			event->accept();
			return;
		}
	}
	QGraphicsPixmapItem::mouseReleaseEvent(event);
}

void MapTileItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	// 先绘制原始图片
	QGraphicsPixmapItem::paint(painter, option, widget);

	// 如果被选中，绘制高亮边框
	if (m_selected)
	{
		painter->save();

		// 绘制选中高亮边框
		QPen pen(QColor(80, 140, 255), 1.5);
		pen.setJoinStyle(Qt::MiterJoin);
		painter->setPen(pen);
		painter->setBrush(QBrush(QColor(80, 140, 255, 20)));

		QRectF rect = boundingRect().adjusted(0.5, 0.5, -0.5, -0.5);
		painter->drawRect(rect);

		// 绘制四角标记
		const qreal cornerSize = CORNER_DRAW_SIZE;
		painter->setPen(Qt::NoPen);

		// 根据当前悬停的角落设置不同颜色
		auto drawCorner = [&](QRectF cornerRect, CornerZone zone) {
			if (m_currentCornerZone == zone)
			{
				// 悬停时使用更亮的颜色
				painter->setBrush(QBrush(QColor(120, 200, 255)));
			}
			else
			{
				painter->setBrush(QBrush(QColor(80, 140, 255)));
			}
			painter->drawRect(cornerRect);
			};

		// 左上角
		drawCorner(QRectF(rect.left(), rect.top(), cornerSize, cornerSize), CornerZone::TopLeft);
		// 右上角
		drawCorner(QRectF(rect.right() - cornerSize, rect.top(), cornerSize, cornerSize), CornerZone::TopRight);
		// 左下角
		drawCorner(QRectF(rect.left(), rect.bottom() - cornerSize, cornerSize, cornerSize), CornerZone::BottomLeft);
		// 右下角
		drawCorner(QRectF(rect.right() - cornerSize, rect.bottom() - cornerSize, cornerSize, cornerSize), CornerZone::BottomRight);

		painter->restore();
	}
}

// ============== 翻转功能 ==============

void MapTileItem::setFlipX(bool flip)
{
	if (m_flipX == flip)
		return;

	m_flipX = flip;
	updateDisplayPixmap();
}

void MapTileItem::setFlipY(bool flip)
{
	if (m_flipY == flip)
		return;

	m_flipY = flip;
	updateDisplayPixmap();
}

void MapTileItem::toggleFlipX()
{
	m_flipX = !m_flipX;
	updateDisplayPixmap();
	qDebug() << "Tile flip X:" << m_flipX;
}

void MapTileItem::toggleFlipY()
{
	m_flipY = !m_flipY;
	updateDisplayPixmap();
	qDebug() << "Tile flip Y:" << m_flipY;
}

void MapTileItem::updateDisplayPixmap()
{
	if (m_originalPixmap.isNull())
		return;

	QPixmap displayPixmap = m_originalPixmap;

	// 应用翻转和旋转变换
	if (m_flipX || m_flipY || m_rotation != 0)
	{
		QTransform transform;

		if (m_flipX || m_flipY)
		{
			transform.scale(m_flipX ? -1 : 1, m_flipY ? -1 : 1);
		}

		if (m_rotation != 0)
		{
			transform.rotate(m_rotation);
		}

		displayPixmap = m_originalPixmap.transformed(transform, Qt::SmoothTransformation);
	}

	setPixmap(displayPixmap);
}

// 旋转
void MapTileItem::setRotation(int degrees)
{
	
	degrees = ((degrees % 360) + 360) % 360;
	degrees = (degrees / 90) * 90;

	if (m_rotation == degrees)
		return;

	m_rotation = degrees;
	updateDisplayPixmap();
}

void MapTileItem::rotateClockwise()
{
	m_rotation = (m_rotation + 90) % 360;
	updateDisplayPixmap();
	qDebug() << "Tile rotated clockwise:" << m_rotation << "degrees";
}

void MapTileItem::rotateCounterClockwise()
{
	m_rotation = (m_rotation - 90 + 360) % 360;
	updateDisplayPixmap();
	qDebug() << "Tile rotated counter-clockwise:" << m_rotation << "degrees";
}
