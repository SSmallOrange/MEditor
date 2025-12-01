#pragma once

#include <QWidget>
#include <QTableWidget>
#include "ui_TilesetBlockWidget.h"

class TilesetBlockWidget : public QWidget
{
	Q_OBJECT
public:
	explicit TilesetBlockWidget(const QString& tilesetId,
		int columns,
		int thumbSize,
		int demoRows,
		QWidget* parent = nullptr)
		: QWidget(parent)
		, m_tilesetId(tilesetId)
	{
		setAttribute(Qt::WA_StyledBackground, true);
		ui.setupUi(this);
		initializeTable(columns, thumbSize);
		ui.labelTitle->setText(tilesetId);
		populateDemo(demoRows, columns);
		connectSignals();
	}

	QString tilesetId() const { return m_tilesetId; }

	// 示例填充，可替换为真实 atlas 切片逻辑
	void populateDemo(int rows, int cols);

	// 预留：真实加载图集
	// void loadAtlas(const QImage& atlas, int tileW, int tileH);

signals:
	void tileSelected(const QString& tilesetId, int tileIndex);
	void removeRequested(const QString& tilesetId);
	void collapsedChanged(const QString& tilesetId, bool collapsed);

private:
	void initializeTable(int columns, int thumbSize);
	void connectSignals();

private:
	Ui::TilesetBlockWidget ui;
	QString m_tilesetId;
};
