#pragma once
#include <QWidget>
#include <QMap>
#include "ui_TilesetsPanel.h"
#include "TilesetBlockWidget.h"
#include "core/SpriteSliceDefine.h"

class TilesetsPanel : public QWidget
{
	Q_OBJECT
public:
	explicit TilesetsPanel(QWidget* parent = nullptr);

	Ui::TilesetsPanel ui;

signals:
	void searchTextChanged(const QString& text);
	void addTilesetRequested();
	void tileSelected(const QString& tilesetId, int tileIndex);
	void tilesetRemoved(const QString& tilesetId);
	void tilesetCollapsedChanged(const QString& tilesetId, bool collapsed);

public slots:
	void onSpriteSheetConfirmed(const SpriteSheetData& data);

private:
	void connectSignals();
	void insertTilesetWidget(TilesetBlockWidget* w);
	void removeTilesetById(const QString& id);

	QMap<QString, TilesetBlockWidget*> m_tilesetBlocks;
};