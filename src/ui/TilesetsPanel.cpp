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

	m_tilesetDataMap.remove(id);
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

	// 保存完整的 SpriteSheetData
	m_tilesetDataMap.insert(tilesetId, data);
}

QVector<SpriteSheetData> TilesetsPanel::getAllTilesetData() const
{
	QVector<SpriteSheetData> result;
	result.reserve(m_tilesetDataMap.size());

	for (auto it = m_tilesetDataMap.constBegin(); it != m_tilesetDataMap.constEnd(); ++it)
	{
		result.append(it.value());
	}

	return result;
}

void TilesetsPanel::clearAllTilesets()
{
	// 清除所有 widget
	for (auto it = m_tilesetBlocks.begin(); it != m_tilesetBlocks.end(); ++it)
	{
		if (auto* block = it.value())
		{
			ui.layoutScroll->removeWidget(block);
			block->deleteLater();
		}
	}

	m_tilesetBlocks.clear();
	m_tilesetDataMap.clear();
}

bool TilesetsPanel::findSliceById(const QString& tilesetId, const QString& sliceId, SpriteSlice& outSlice, QPixmap& outPixmap) const
{
	if (!m_tilesetDataMap.contains(tilesetId))
		return false;

	const SpriteSheetData& data = m_tilesetDataMap[tilesetId];

	for (const SpriteSlice& slice : data.slices)
	{
		if (slice.id.toString(QUuid::WithoutBraces) == sliceId)
		{
			outSlice = slice;
			outPixmap = data.pixmap.copy(slice.x, slice.y, slice.width, slice.height);
			return true;
		}
	}

	return false;
}