#include "TilesetsPanel.h"
#include "TilesetBlockWidget.h"
#include <QToolButton>
#include <QLineEdit>

void TilesetsPanel::connectSignals()
{
	connect(ui.editSearch, &QLineEdit::textChanged, this, &TilesetsPanel::searchTextChanged);
	connect(ui.buttonAddTileset, &QToolButton::clicked, this, &TilesetsPanel::onAddTileset);
}

void TilesetsPanel::insertTilesetWidget(TilesetBlockWidget* w)
{
	// 插入到状态和尾部 spacer 之前（假设最后两个是 status + spacer）
	int insertPos = ui.layoutScroll->count() - 2;
	if (insertPos < 0) insertPos = ui.layoutScroll->count();
	ui.layoutScroll->insertWidget(insertPos, w);

	// 连接子控件信号
	connect(w, &TilesetBlockWidget::tileSelected, this, &TilesetsPanel::tileSelected);
	connect(w, &TilesetBlockWidget::removeRequested, this, [this](const QString& id) {
		removeTilesetById(id);
		emit tilesetRemoved(id);
		});
	connect(w, &TilesetBlockWidget::collapsedChanged, this, &TilesetsPanel::tilesetCollapsedChanged);
}

void TilesetsPanel::removeTilesetById(const QString& id)
{
	for (int i = 0; i < ui.layoutScroll->count(); ++i)
	{
		auto* item = ui.layoutScroll->itemAt(i);
		if (auto* w = item->widget())
		{
			auto* block = qobject_cast<TilesetBlockWidget*>(w);
			if (block && block->tilesetId() == id)
			{
				ui.layoutScroll->takeAt(i);
				block->deleteLater();
				return;
			}
		}
	}
}

void TilesetsPanel::onAddTileset()
{
	static int counter = 1;
	const QString id = QString("Tileset %1").arg(counter++);
	const int columns = 8;
	const int thumbSize = 40;
	const int demoRows = 3;

	auto* w = new TilesetBlockWidget(id, columns, thumbSize, demoRows, this);
	insertTilesetWidget(w);

	emit addTilesetRequested();
}