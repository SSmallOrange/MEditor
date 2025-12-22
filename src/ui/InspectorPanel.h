#pragma once

#include "core/SpriteSliceDefine.h"

#include <QWidget>
#include "ui_InspectorPanel.h"

class MapTileItem;
struct SpriteSlice;

class InspectorPanel : public QWidget
{
	Q_OBJECT
public:
	explicit InspectorPanel(QWidget* parent = nullptr);

	// 显示瓦片信息
	void showTileInfo(MapTileItem* tile);

	// 清空显示
	void clearInfo();

	// 设置是否启用编辑
	void setEditEnabled(bool enabled);

signals:
	// 属性变化信号（用于同步更新 MapTileItem）
	void positionChanged(int x, int y);
	void layerChanged(int layer);
	void nameChanged(const QString& name);
	void collisionTypeChanged(CollisionType type);
	void tagsChanged(const QString& tags);

private slots:
	void onAddTagClicked();
	void onRemoveTagClicked();

private:
	void setupConnections();
	void initComboBoxes();
	void blockAllSignals(bool block);
	void updateTagsFromList();

private:
	Ui::InspectorPanel ui;
	MapTileItem* m_currentTile = nullptr;
	bool m_updating = false;  // 防止信号循环
};