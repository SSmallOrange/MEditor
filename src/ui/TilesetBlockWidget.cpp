

#include "TilesetBlockWidget.h"
#include <QToolButton>
#include <QHeaderView>
#include <QTableWidgetItem>

void TilesetBlockWidget::initializeTable(int columns, int thumbSize)
{
	auto* table = ui.tableTiles;
	table->setColumnCount(columns);
	table->horizontalHeader()->setVisible(false);
	table->verticalHeader()->setVisible(false);
	table->setIconSize(QSize(thumbSize, thumbSize));
	table->setFocusPolicy(Qt::NoFocus);
	table->setSelectionMode(QAbstractItemView::SingleSelection);
	table->setSelectionBehavior(QAbstractItemView::SelectItems);
	table->setShowGrid(false);
	table->setFrameShape(QFrame::NoFrame);
	table->setStyleSheet("QTableWidget { background: transparent; } QTableWidget::item { border:0; }");
}

void TilesetBlockWidget::populateDemo(int rows, int cols)
{
	auto* table = ui.tableTiles;
	table->setRowCount(rows);
	table->setColumnCount(cols);
	for (int r = 0; r < rows; ++r)
	{
		for (int c = 0; c < cols; ++c)
		{
			auto* item = new QTableWidgetItem;
			item->setToolTip(QString("Tile %1").arg(r * cols + c));
			// TODO: item->setIcon(QIcon(":/tiles/grass.png"));
			table->setItem(r, c, item);
		}
	}
}

void TilesetBlockWidget::connectSignals()
{
	// Tile 点击
	connect(ui.tableTiles, &QTableWidget::itemClicked, this, [this](QTableWidgetItem* item) {
		const int cols = ui.tableTiles->columnCount();
		const int index = item->row() * cols + item->column();
		emit tileSelected(m_tilesetId, index);
		});

	// 折叠
	connect(ui.buttonCollapse, &QToolButton::toggled, this, [this](bool checked) {
		ui.tableTiles->setVisible(!checked);
		ui.buttonCollapse->setText(checked ? "▸" : "▾");
		setProperty("collapsed", checked);
		style()->unpolish(this);
		style()->polish(this);
		emit collapsedChanged(m_tilesetId, checked);
		});

	// 删除
	connect(ui.buttonRemove, &QToolButton::clicked, this, [this]() {
		emit removeRequested(m_tilesetId);
		});
}