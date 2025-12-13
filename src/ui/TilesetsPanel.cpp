#include "ui/Common.h"
#include "TilesetsPanel.h"
#include "TilesetBlockWidget.h"
#include <QToolButton>

TilesetsPanel::TilesetsPanel(QWidget* parent)
	: QWidget(parent)
{
	setAttribute(Qt::WA_StyledBackground, true);
	ui.setupUi(this);
	LOAD_QSS(":/TilesetsPanel/TilesetsPanel.qss");

	auto icon = QIcon(":/TilesetsPanel/search.png");
	ui.editSearch->addAction(icon, QLineEdit::LeadingPosition);
	ui.editSearch->setClearButtonEnabled(true);

	connectSignals();
}

void TilesetsPanel::connectSignals()
{
	connect(ui.editSearch, &QLineEdit::textChanged, this, &TilesetsPanel::searchTextChanged);
	connect(ui.buttonAddTileset, &QToolButton::clicked, this, &TilesetsPanel::addTilesetRequested);
}

void TilesetsPanel::insertTilesetWidget(TilesetBlockWidget* w)
{
	int insertPos = ui.layoutScroll->count() - 1;
	if (insertPos < 0) insertPos = ui.layoutScroll->count();
	ui.layoutScroll->insertWidget(insertPos, w);

	connect(w, &TilesetBlockWidget::tileSelected, this, &TilesetsPanel::tileSelected);
	connect(w, &TilesetBlockWidget::removeRequested, this, [this](const QString& id) {
		removeTilesetById(id);
		emit tilesetRemoved(id);
		});
	connect(w, &TilesetBlockWidget::collapsedChanged, this, &TilesetsPanel::tilesetCollapsedChanged);
}

void TilesetsPanel::removeTilesetById(const QString& id)
{
	if (!m_tilesetBlocks.contains(id))
		return;

	if (auto* block = m_tilesetBlocks.take(id))
	{
		ui.layoutScroll->removeWidget(block);
		block->deleteLater();
	}
}

void TilesetsPanel::onSpriteSheetConfirmed(const SpriteSheetData& data)
{
	const QString tilesetId = data.fileName.isEmpty() ? QStringLiteral("Tileset") : data.fileName;
	const int columns = 8;
	const int thumbSize = 40;

	auto* widget = new TilesetBlockWidget(tilesetId, columns, thumbSize, 1, this);
	widget->setTilesetData(data.pixmap, data.slices, thumbSize);

	insertTilesetWidget(widget);
	m_tilesetBlocks.insert(tilesetId, widget);
}