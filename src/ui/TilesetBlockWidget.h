#pragma once

#include <QWidget>
#include <QListWidget>
#include "ui_TilesetBlockWidget.h"
#include "core/SpriteSliceDefine.h"

class TilesetBlockWidget : public QWidget
{
	Q_OBJECT
public:
	explicit TilesetBlockWidget(const QString& tilesetId,
		int columns,
		int thumbSize,
		int demoRows,
		QWidget* parent = nullptr);

	QString tilesetId() const { return m_tilesetId; }
	void setTilesetData(const QPixmap& atlas, const QVector<SpriteSlice>& slices, int thumbSize);
	void setCollapsed(bool collapsed);
	bool isCollapsed() const { return m_collapsed; }

	// 获取指定索引的切片数据
	SpriteSlice getSlice(int index) const;
	QPixmap getSlicePixmap(int index) const;

signals:
	void tileSelected(const QString& tilesetId, int tileIndex);
	void removeRequested(const QString& tilesetId);
	void collapsedChanged(const QString& tilesetId, bool collapsed);

private:
	void initializeList(int thumbSize);
	void connectSignals();
	void populateDemo(int rows, int cols, int thumbSize);
	void updateCollapsedState();
	void setupDragDrop();

	// 拖拽事件处理
	bool eventFilter(QObject* watched, QEvent* event) override;
	void startDrag(QListWidgetItem* item);

private:
	Ui::TilesetBlockWidget ui;
	QString m_tilesetId;
	QPixmap m_atlas;
	QVector<SpriteSlice> m_slices;
	int m_thumbSize = 32;
	bool m_collapsed = false;

	// 拖拽相关
	QPoint m_dragStartPos;
	QListWidgetItem* m_dragItem = nullptr;
};