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

	// 获取所有已加载的图集数据（用于导出）
	QVector<SpriteSheetData> getAllTilesetData() const;

	// 清除所有图集
	void clearAllTilesets();

	// 根据 tilesetId 查找切片
	bool findSliceById(const QString& tilesetId, const QString& sliceId, SpriteSlice& outSlice, QPixmap& outPixmap) const;

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
	QMap<QString, SpriteSheetData> m_tilesetDataMap;  // 保存完整的 SpriteSheetData
};