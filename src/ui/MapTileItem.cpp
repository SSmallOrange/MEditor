#include "MapTileItem.h"
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QStyleOptionGraphicsItem>
#include <QApplication>

MapTileItem::MapTileItem(const QPixmap& pixmap, const SpriteSlice& slice,
	int gridX, int gridY, int layer, QGraphicsItem* parent)
	: QGraphicsPixmapItem(pixmap, parent)
	, m_slice(slice)
	, m_gridX(gridX)
	, m_gridY(gridY)
	, m_layer(layer)
{
	setTransformationMode(Qt::SmoothTransformation);
	setAcceptedMouseButtons(Qt::LeftButton);
	setFlag(QGraphicsItem::ItemIsSelectable, false);
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

void MapTileItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		m_dragStartPos = event->scenePos();
		m_originalPos = pos();
		emit clicked(this);
		event->accept();
		return;
	}
	QGraphicsPixmapItem::mousePressEvent(event);
}

void MapTileItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
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
	if (event->button() == Qt::LeftButton && m_dragging)
	{
		m_dragging = false;
		setOpacity(1.0);  // 恢复不透明
		emit dragFinished(this, event->scenePos());
		event->accept();
		return;
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

		// 绘制选中高亮边框（细边框）
		QPen pen(QColor(80, 140, 255), 1.5);
		pen.setJoinStyle(Qt::MiterJoin);
		painter->setPen(pen);
		painter->setBrush(QBrush(QColor(80, 140, 255, 20)));

		QRectF rect = boundingRect().adjusted(0.5, 0.5, -0.5, -0.5);
		painter->drawRect(rect);

		// 绘制四角标记（更小的角标）
		const qreal cornerSize = 5;
		painter->setBrush(QBrush(QColor(80, 140, 255)));
		painter->setPen(Qt::NoPen);

		// 左上角
		painter->drawRect(QRectF(rect.left(), rect.top(), cornerSize, cornerSize));
		// 右上角
		painter->drawRect(QRectF(rect.right() - cornerSize, rect.top(), cornerSize, cornerSize));
		// 左下角
		painter->drawRect(QRectF(rect.left(), rect.bottom() - cornerSize, cornerSize, cornerSize));
		// 右下角
		painter->drawRect(QRectF(rect.right() - cornerSize, rect.bottom() - cornerSize, cornerSize, cornerSize));

		painter->restore();
	}
}