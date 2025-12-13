#include "TilesetBlockWidget.h"
#include "core/TileDragData.h"
#include <QToolButton>
#include <QListWidgetItem>
#include <QDrag>
#include <QMouseEvent>
#include <QApplication>

TilesetBlockWidget::TilesetBlockWidget(const QString& tilesetId,
	int columns,
	int thumbSize,
	int demoRows,
	QWidget* parent)
	: QWidget(parent)
	, m_tilesetId(tilesetId)
	, m_thumbSize(thumbSize)
{
	setAttribute(Qt::WA_StyledBackground, true);
	ui.setupUi(this);
	initializeList(thumbSize);
	ui.labelTitle->setText(tilesetId);
	populateDemo(demoRows, columns, thumbSize);
	connectSignals();
	setupDragDrop();
}

void TilesetBlockWidget::initializeList(int thumbSize)
{
	auto* list = ui.listTiles;
	list->setViewMode(QListView::IconMode);
	list->setFlow(QListView::LeftToRight);
	list->setWrapping(true);
	list->setResizeMode(QListView::Adjust);
	list->setMovement(QListView::Static);
	list->setSpacing(4);
	list->setFocusPolicy(Qt::NoFocus);
	list->setSelectionMode(QAbstractItemView::SingleSelection);
	list->setFrameShape(QFrame::NoFrame);
	list->setEditTriggers(QAbstractItemView::NoEditTriggers);
	list->setIconSize(QSize(thumbSize, thumbSize));
	list->setStyleSheet(QStringLiteral(
		"QListWidget { background: transparent; }"
		"QListWidget::item { border: 0; padding: 2px; }"));
}

void TilesetBlockWidget::setupDragDrop()
{
	// 启用拖拽
	ui.listTiles->setDragEnabled(true);
	ui.listTiles->setDragDropMode(QAbstractItemView::DragOnly);
	ui.listTiles->viewport()->installEventFilter(this);
}

bool TilesetBlockWidget::eventFilter(QObject* watched, QEvent* event)
{
	if (watched == ui.listTiles->viewport())
	{
		if (event->type() == QEvent::MouseButtonPress)
		{
			auto* me = static_cast<QMouseEvent*>(event);
			if (me->button() == Qt::LeftButton)
			{
				m_dragStartPos = me->pos();
				m_dragItem = ui.listTiles->itemAt(me->pos());
			}
		}
		else if (event->type() == QEvent::MouseMove)
		{
			auto* me = static_cast<QMouseEvent*>(event);
			if ((me->buttons() & Qt::LeftButton) && m_dragItem)
			{
				int distance = (me->pos() - m_dragStartPos).manhattanLength();
				if (distance >= QApplication::startDragDistance())
				{
					startDrag(m_dragItem);
					m_dragItem = nullptr;
					return true;
				}
			}
		}
		else if (event->type() == QEvent::MouseButtonRelease)
		{
			m_dragItem = nullptr;
		}
	}
	return QWidget::eventFilter(watched, event);
}

void TilesetBlockWidget::startDrag(QListWidgetItem* item)
{
	if (!item)
		return;

	int index = item->data(Qt::UserRole).toInt();
	if (index < 0 || index >= m_slices.size())
		return;

	// 创建拖拽数据
	TileDragData dragData;
	dragData.tilesetId = m_tilesetId;
	dragData.sliceIndex = index;
	dragData.slice = m_slices[index];
	dragData.pixmap = getSlicePixmap(index);

	auto* mimeData = new TileMimeData(dragData);

	// 创建拖拽对象
	auto* drag = new QDrag(this);
	drag->setMimeData(mimeData);

	// 设置拖拽时的缩略图
	QPixmap dragPixmap = dragData.pixmap.scaled(
		qMin(64, dragData.pixmap.width()),
		qMin(64, dragData.pixmap.height()),
		Qt::KeepAspectRatio,
		Qt::SmoothTransformation
	);
	drag->setPixmap(dragPixmap);
	drag->setHotSpot(QPoint(dragPixmap.width() / 2, dragPixmap.height() / 2));

	// 执行拖拽
	drag->exec(Qt::CopyAction);
}

SpriteSlice TilesetBlockWidget::getSlice(int index) const
{
	if (index >= 0 && index < m_slices.size())
		return m_slices[index];
	return SpriteSlice();
}

QPixmap TilesetBlockWidget::getSlicePixmap(int index) const
{
	if (index < 0 || index >= m_slices.size() || m_atlas.isNull())
		return QPixmap();

	const auto& slice = m_slices[index];
	return m_atlas.copy(slice.x, slice.y, slice.width, slice.height);
}

void TilesetBlockWidget::populateDemo(int rows, int cols, int thumbSize)
{
	auto* list = ui.listTiles;
	list->clear();
	const int count = rows * cols;
	for (int i = 0; i < count; ++i)
	{
		auto* item = new QListWidgetItem;
		item->setToolTip(QStringLiteral("Tile %1").arg(i));
		item->setSizeHint(QSize(thumbSize, thumbSize));
		item->setFlags(item->flags() & ~Qt::ItemIsEditable);
		list->addItem(item);
	}
}

void TilesetBlockWidget::setTilesetData(const QPixmap& atlas, const QVector<SpriteSlice>& slices, int thumbSize)
{
	m_atlas = atlas;
	m_slices = slices;
	m_thumbSize = thumbSize;

	if (atlas.isNull() || slices.isEmpty())
		return;

	auto* list = ui.listTiles;
	list->clear();

	// 计算最大尺寸用于 iconSize
	int maxWidth = thumbSize;
	int maxHeight = thumbSize;

	for (const auto& slice : slices)
	{
		const int maxDim = thumbSize * 3;
		QSize originalSize(slice.width, slice.height);
		QSize scaledSize = originalSize;

		if (slice.width > maxDim || slice.height > maxDim)
		{
			scaledSize = originalSize.scaled(maxDim, maxDim, Qt::KeepAspectRatio);
		}

		maxWidth = qMax(maxWidth, scaledSize.width());
		maxHeight = qMax(maxHeight, scaledSize.height());
	}

	list->setIconSize(QSize(maxWidth, maxHeight));

	// 填充精灵图
	for (int i = 0; i < slices.size(); ++i)
	{
		const auto& slice = slices[i];

		const int maxDim = thumbSize * 3;
		QSize originalSize(slice.width, slice.height);
		QSize displaySize = originalSize;

		if (slice.width > maxDim || slice.height > maxDim)
		{
			displaySize = originalSize.scaled(maxDim, maxDim, Qt::KeepAspectRatio);
		}

		QPixmap tile = atlas.copy(slice.x, slice.y, slice.width, slice.height)
			.scaled(displaySize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

		auto* item = new QListWidgetItem;
		item->setIcon(QIcon(tile));
		item->setToolTip(QStringLiteral("%1\n(%2,%3) %4x%5")
			.arg(slice.name)
			.arg(slice.x)
			.arg(slice.y)
			.arg(slice.width)
			.arg(slice.height));
		item->setData(Qt::UserRole, i);
		item->setFlags(item->flags() & ~Qt::ItemIsEditable);
		item->setSizeHint(QSize(displaySize.width() + 8, displaySize.height() + 8));
		list->addItem(item);
	}
}

void TilesetBlockWidget::setCollapsed(bool collapsed)
{
	if (m_collapsed == collapsed)
		return;

	m_collapsed = collapsed;
	ui.buttonCollapse->setChecked(collapsed);
	updateCollapsedState();
}

void TilesetBlockWidget::updateCollapsedState()
{
	ui.listTiles->setVisible(!m_collapsed);
	ui.buttonCollapse->setText(m_collapsed ? QStringLiteral("▸") : QStringLiteral("▾"));

	if (m_collapsed)
	{
		setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
		setFixedHeight(ui.layoutHeader->sizeHint().height() + ui.layoutRoot->contentsMargins().top() + ui.layoutRoot->contentsMargins().bottom());
	}
	else
	{
		setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
		setMinimumHeight(0);
		setMaximumHeight(QWIDGETSIZE_MAX);
	}

	updateGeometry();
}

void TilesetBlockWidget::connectSignals()
{
	connect(ui.listTiles, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
		const int index = item->data(Qt::UserRole).toInt();
		emit tileSelected(m_tilesetId, index);
		});

	connect(ui.buttonCollapse, &QToolButton::toggled, this, [this](bool checked) {
		m_collapsed = checked;
		updateCollapsedState();
		emit collapsedChanged(m_tilesetId, checked);
		});

	connect(ui.buttonRemove, &QToolButton::clicked, this, [this]() {
		emit removeRequested(m_tilesetId);
		});
}