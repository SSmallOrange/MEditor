#include "SpriteSliceGraphicsView.h"
#include <QGraphicsScene>

SpriteSliceGraphicsView::SpriteSliceGraphicsView(QWidget* parent)
	: QGraphicsView(parent)
{
}

void SpriteSliceGraphicsView::setManualSelectionEnabled(bool enabled)
{
	m_manualSelectionEnabled = enabled;

	if (enabled)
	{
		// 手动模式：禁用拖拽滚动，启用框选
		setDragMode(QGraphicsView::NoDrag);
		setCursor(Qt::CrossCursor);
	}
	else
	{
		// 非手动模式：启用拖拽滚动
		setDragMode(QGraphicsView::ScrollHandDrag);
		setCursor(Qt::ArrowCursor);

		// 清除正在进行的选择
		if (m_selectionRectItem && scene())
		{
			scene()->removeItem(m_selectionRectItem);
			delete m_selectionRectItem;
			m_selectionRectItem = nullptr;
		}
		m_isSelecting = false;
		m_isDragging = false;
	}
}

void SpriteSliceGraphicsView::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		m_pressPos = mapToScene(event->pos());

		if (m_manualSelectionEnabled)
		{
			m_isSelecting = true;
			m_isDragging = false;
			m_selectionStart = m_pressPos;

			// 创建选择矩形（但暂不显示，等确认是拖拽后再显示）
			if (!m_selectionRectItem)
			{
				m_selectionRectItem = new QGraphicsRectItem();
				m_selectionRectItem->setPen(QPen(QColor(0, 150, 255), 2, Qt::DashLine));
				m_selectionRectItem->setBrush(QBrush(QColor(0, 150, 255, 50)));
				m_selectionRectItem->setZValue(200);
				scene()->addItem(m_selectionRectItem);
			}

			m_selectionRectItem->setRect(QRectF(m_selectionStart, QSizeF(0, 0)));
			m_selectionRectItem->setVisible(false);  // 初始不显示

			event->accept();
			return;
		}
	}

	QGraphicsView::mousePressEvent(event);
}

void SpriteSliceGraphicsView::mouseMoveEvent(QMouseEvent* event)
{
	QPointF scenePos = mapToScene(event->pos());
	emit mousePosChanged(scenePos);

	if (m_manualSelectionEnabled && m_isSelecting)
	{
		// 计算移动距离，判断是否开始拖拽
		QPointF delta = scenePos - m_selectionStart;
		double distance = qSqrt(delta.x() * delta.x() + delta.y() * delta.y());

		if (!m_isDragging && distance > DRAG_THRESHOLD)
		{
			// 超过阈值，开始拖拽框选
			m_isDragging = true;
			if (m_selectionRectItem)
			{
				m_selectionRectItem->setVisible(true);
			}
		}

		if (m_isDragging)
		{
			updateSelectionRect(scenePos);
		}

		event->accept();
		return;
	}

	QGraphicsView::mouseMoveEvent(event);
}

void SpriteSliceGraphicsView::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		QPointF releasePos = mapToScene(event->pos());

		if (m_manualSelectionEnabled && m_isSelecting)
		{
			m_isSelecting = false;

			if (m_isDragging)
			{
				// 拖拽模式：完成框选
				m_isDragging = false;
				updateSelectionRect(releasePos);

				if (m_selectionRectItem)
				{
					QRectF finalRect = m_selectionRectItem->rect().normalized();
					m_selectionRectItem->setVisible(false);

					if (finalRect.width() > 5 && finalRect.height() > 5)
					{
						emit selectionRectFinished(finalRect);
					}
				}
			}
			else
			{
				// 点击模式：发送点击信号
				emit clickedAt(releasePos);
			}

			event->accept();
			return;
		}
		else if (!m_manualSelectionEnabled)
		{
			// 非手动模式下也支持点击选中
			// 检查是否是简单点击（没有明显移动）
			QPointF delta = releasePos - m_pressPos;
			double distance = qSqrt(delta.x() * delta.x() + delta.y() * delta.y());

			if (distance < DRAG_THRESHOLD)
			{
				emit clickedAt(releasePos);
			}
		}
	}

	QGraphicsView::mouseReleaseEvent(event);
}

void SpriteSliceGraphicsView::updateSelectionRect(const QPointF& endPos)
{
	if (m_selectionRectItem)
	{
		QRectF rect(m_selectionStart, endPos);
		m_selectionRectItem->setRect(rect.normalized());
	}
}