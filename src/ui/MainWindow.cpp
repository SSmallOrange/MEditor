#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "MapViewWidget.h"
#include "MapTileItem.h"
#include "TitleBarWidget.h"
#include "InspectorPanel.h"

#include "app/AppContext.h"
#include "app/DocumentManager.h"
#include "core/MapExporter.h"

#include <QMenuBar>
#include <QStatusBar>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <QApplication>
#include <QShortcut>
#include <QKeySequence>
#include <QGraphicsDropShadowEffect>
#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(AppContext* ctx, QWidget* parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, m_ctx(ctx)
{
	ui->setupUi(this);

	// 无边框窗口（去掉系统标题栏）
	setWindowFlag(Qt::FramelessWindowHint, true);
	setAttribute(Qt::WA_TranslucentBackground, true);

	SetupUi();
	SetupConnections();

	// 默认新建一张地图
	m_ctx->documentManager.newDefaultDocument();
	ui->mapViewWidget->updateMap();

	// 初始化 UI 显示（从 MapViewWidget 读取初始值显示到控件）
	InitMapViewUI();
}

void MainWindow::SetupUi()
{
	ui->mapViewWidget->SetContext(m_ctx);

	// 初始化图层下拉框
	ui->comboBoxLayer->clear();
	ui->comboBoxLayer->addItem(QStringLiteral("图层 0 (背景)"), 0);
	ui->comboBoxLayer->addItem(QStringLiteral("图层 1 (地面)"), 1);
	ui->comboBoxLayer->addItem(QStringLiteral("图层 2 (装饰)"), 2);
	ui->comboBoxLayer->addItem(QStringLiteral("图层 3 (前景)"), 3);

	// 默认显示栅格
	ui->checkBoxGrid->setChecked(true);
}

void MainWindow::SetupStatusBar()
{
}

void MainWindow::SetupConnections()
{
	QObject::connect(&m_ctx->documentManager, &DocumentManager::documentChanged,
		ui->mapViewWidget, &MapViewWidget::updateMap);

	QObject::connect(&m_ctx->documentManager, &DocumentManager::documentChanged,
		this, &MainWindow::InitMapViewUI);

	auto newShortcut = new QShortcut(QKeySequence::New, this);
	QObject::connect(newShortcut, &QShortcut::activated, this, [this]() {
		OnNewMap();
		});

	// 导出快捷键 Ctrl+E
	auto exportShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_E), this);
	QObject::connect(exportShortcut, &QShortcut::activated, this, &MainWindow::onExportMap);

	connect(ui->TilesetsPanelWidget, &TilesetsPanel::addTilesetRequested, this, &MainWindow::SlotSwitchSpriteSliceWidget);
	connect(ui->spriteSliceEditorWidget, &SpriteSliceEditorWidget::SignalReturnToMainPanel, this, &MainWindow::SlotSwitchMainWidget);
	connect(ui->spriteSliceEditorWidget, &SpriteSliceEditorWidget::SignalSpriteSheetConfirmed, ui->TilesetsPanelWidget, &TilesetsPanel::onSpriteSheetConfirmed);

	// 设置 MapView 控制连接
	SetupMapViewConnections();

	// 设置 Inspector 连接
	SetupInspectorConnections();

	// 设置标题栏连接
	SetupTitleBarConnections();
}

void MainWindow::SetupTitleBarConnections()
{
	connect(ui->toolBarWidget, &TitleBarWidget::exportRequested, this, &MainWindow::onExportMap);
	connect(ui->toolBarWidget, &TitleBarWidget::restartRequested, this, &MainWindow::onResetMap);
	connect(ui->toolBarWidget, &TitleBarWidget::saveRequested, this, &MainWindow::onSaveMap);
}

void MainWindow::SetupMapViewConnections()
{
	// ========== UI 控件 -> MapViewWidget ==========

	// 栅格显示控制
	connect(ui->checkBoxGrid, &QCheckBox::toggled, this, &MainWindow::onGridVisibleChanged);

	// 栅格尺寸控制
	connect(ui->spinboxGridWidth, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onGridSizeChanged);
	connect(ui->spinboxGridHeight, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onGridSizeChanged);

	// 地图尺寸控制
	connect(ui->spinboxMapWidth, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onMapSizeChanged);
	connect(ui->spinboxMapHeight, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onMapSizeChanged);

	// 缩放滑块
	connect(ui->zoomSlider, &QSlider::valueChanged, this, &MainWindow::onZoomSliderChanged);

	// 图层选择
	connect(ui->comboBoxLayer, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onLayerChanged);

	// ========== MapViewWidget -> UI 控件 ==========
	connect(ui->mapViewWidget, &MapViewWidget::zoomChanged, this, [this](int percent) {
		ui->zoomSlider->blockSignals(true);
		ui->zoomSlider->setValue(percent);
		ui->zoomSlider->blockSignals(false);
		ui->zoomLabel->setText(QString("%1%").arg(percent));
		});

	// 瓦片选中信号
	connect(ui->mapViewWidget, &MapViewWidget::tileSelected, this, &MainWindow::onTileSelected);
	connect(ui->mapViewWidget, &MapViewWidget::tileDeselected, this, &MainWindow::onTileDeselected);
}

void MainWindow::SetupInspectorConnections()
{
	// Inspector 属性变化 -> MapViewWidget
	connect(ui->inspectorPanel, &InspectorPanel::positionChanged, this, &MainWindow::onInspectorPositionChanged);
	connect(ui->inspectorPanel, &InspectorPanel::layerChanged, this, &MainWindow::onInspectorLayerChanged);
}

void MainWindow::InitMapViewUI()
{
	if (!ui->mapViewWidget)
		return;

	// 阻止信号触发，仅初始化显示值
	ui->spinboxGridWidth->blockSignals(true);
	ui->spinboxGridHeight->blockSignals(true);
	ui->spinboxMapWidth->blockSignals(true);
	ui->spinboxMapHeight->blockSignals(true);
	ui->zoomSlider->blockSignals(true);
	ui->checkBoxGrid->blockSignals(true);

	// 从 MapViewWidget 读取当前值并显示
	ui->spinboxGridWidth->setValue(ui->mapViewWidget->tileWidth());
	ui->spinboxGridHeight->setValue(ui->mapViewWidget->tileHeight());
	ui->spinboxMapWidth->setValue(ui->mapViewWidget->mapWidth());
	ui->spinboxMapHeight->setValue(ui->mapViewWidget->mapHeight());
	ui->checkBoxGrid->setChecked(ui->mapViewWidget->isGridVisible());

	int zoomPercent = ui->mapViewWidget->zoomPercent();
	ui->zoomSlider->setValue(zoomPercent);
	ui->zoomLabel->setText(QString("%1%").arg(zoomPercent));

	// 恢复信号
	ui->spinboxGridWidth->blockSignals(false);
	ui->spinboxGridHeight->blockSignals(false);
	ui->spinboxMapWidth->blockSignals(false);
	ui->spinboxMapHeight->blockSignals(false);
	ui->zoomSlider->blockSignals(false);
	ui->checkBoxGrid->blockSignals(false);
}

void MainWindow::onGridVisibleChanged(bool checked)
{
	ui->mapViewWidget->setGridVisible(checked);
}

void MainWindow::onGridSizeChanged()
{
	int width = ui->spinboxGridWidth->value();
	int height = ui->spinboxGridHeight->value();

	if (width > 0 && height > 0)
	{
		ui->mapViewWidget->setGridSize(width, height);
	}
}

void MainWindow::onMapSizeChanged()
{
	int width = ui->spinboxMapWidth->value();
	int height = ui->spinboxMapHeight->value();

	if (width > 0 && height > 0)
	{
		ui->mapViewWidget->setMapSize(width, height);
	}
}

void MainWindow::onZoomSliderChanged(int value)
{
	ui->mapViewWidget->setZoomPercent(value);
	ui->zoomLabel->setText(QString("%1%").arg(value));
}

void MainWindow::onLayerChanged(int index)
{
	int layer = ui->comboBoxLayer->itemData(index).toInt();
	ui->mapViewWidget->setCurrentLayer(layer);
}

// ========== 瓦片选中相关 ==========

void MainWindow::onTileSelected(MapTileItem* tile)
{
	if (tile)
	{
		ui->inspectorPanel->showTileInfo(tile);
		ui->label->setText(QString("选中: %1 @ (%2, %3)")
			.arg(tile->slice().name)
			.arg(tile->gridX())
			.arg(tile->gridY()));
	}
}

void MainWindow::onTileDeselected()
{
	ui->inspectorPanel->clearInfo();
	ui->label->setText(QStringLiteral("Ready"));
}

// ========== Inspector 属性变化 ==========

void MainWindow::onInspectorPositionChanged(int x, int y)
{
	MapTileItem* tile = ui->mapViewWidget->selectedTile();
	if (!tile)
		return;

	// 检查边界
	int mapWidth = ui->mapViewWidget->mapWidth();
	int mapHeight = ui->mapViewWidget->mapHeight();

	if (x < 0 || y < 0 ||
		x + tile->gridWidth() > mapWidth ||
		y + tile->gridHeight() > mapHeight)
	{
		// 超出边界，恢复原值
		ui->inspectorPanel->showTileInfo(tile);
		return;
	}

	// 更新瓦片位置
	tile->setGridPos(x, y);

	int tileWidth = ui->mapViewWidget->tileWidth();
	int tileHeight = ui->mapViewWidget->tileHeight();
	tile->setPos(x * tileWidth, y * tileHeight);

	ui->label->setText(QString("移动到: (%1, %2)").arg(x).arg(y));
}

void MainWindow::onInspectorLayerChanged(int layer)
{
	MapTileItem* tile = ui->mapViewWidget->selectedTile();
	if (!tile)
		return;

	// 更新瓦片图层
	tile->setLayer(layer);
	tile->setZValue(10 + layer);

	// 由于图层变化，需要取消选中（因为当前图层可能不匹配了）
	int currentLayer = ui->mapViewWidget->currentLayer();
	if (layer != currentLayer)
	{
		ui->mapViewWidget->clearSelection();
	}

	qDebug() << "Tile layer changed to:" << layer;
}

// ========== 标题栏按钮 ==========

void MainWindow::onExportMap()
{
	// 获取地图文档
	const MapDocument* doc = m_ctx->documentManager.document();
	if (!doc)
	{
		QMessageBox::warning(this, QStringLiteral("导出失败"), QStringLiteral("没有可导出的地图文档"));
		return;
	}

	// 获取所有瓦片
	const auto& tiles = ui->mapViewWidget->placedTiles();
	if (tiles.isEmpty())
	{
		QMessageBox::StandardButton reply = QMessageBox::question(
			this,
			QStringLiteral("确认导出"),
			QStringLiteral("地图中没有放置任何瓦片，是否继续导出空地图？"),
			QMessageBox::Yes | QMessageBox::No
		);

		if (reply != QMessageBox::Yes)
			return;
	}

	// 打开文件保存对话框
	QString defaultName = doc->name.isEmpty() ? "untitled_map" : doc->name;
	QString filePath = QFileDialog::getSaveFileName(
		this,
		QStringLiteral("导出地图"),
		defaultName + ".json",
		QStringLiteral("JSON 文件 (*.json);;所有文件 (*.*)")
	);

	if (filePath.isEmpty())
		return;

	// 确保文件扩展名
	if (!filePath.endsWith(".json", Qt::CaseInsensitive))
	{
		filePath += ".json";
	}

	// 执行导出
	ui->label->setText(QStringLiteral("正在导出..."));
	QApplication::processEvents();

	MapExporter::ExportOptions options;
	options.prettyPrint = true;

	bool success = MapExporter::exportToJson(
		filePath,
		doc,
		tiles,
		ui->mapViewWidget->tileWidth(),
		ui->mapViewWidget->tileHeight(),
		options
	);

	if (success)
	{
		ui->label->setText(QStringLiteral("导出成功: %1").arg(filePath));
		QMessageBox::information(
			this,
			QStringLiteral("导出成功"),
			QString("地图已成功导出到:\n%1\n\n共导出 %2 个瓦片")
			.arg(filePath)
			.arg(tiles.size())
		);
	}
	else
	{
		ui->label->setText(QStringLiteral("导出失败"));
		QMessageBox::critical(
			this,
			QStringLiteral("导出失败"),
			QString("导出失败: %1").arg(MapExporter::lastError())
		);
	}
}

void MainWindow::onResetMap()
{
	QMessageBox::StandardButton reply = QMessageBox::question(
		this,
		QStringLiteral("确认重置"),
		QStringLiteral("确定要重置地图吗？这将清除所有已放置的瓦片。"),
		QMessageBox::Yes | QMessageBox::No
	);

	if (reply == QMessageBox::Yes)
	{
		ui->mapViewWidget->clearAllTiles();
		ui->inspectorPanel->clearInfo();
		ui->label->setText(QStringLiteral("地图已重置"));
	}
}

void MainWindow::onSaveMap()
{
	// 保存功能（暂时与导出相同，后续可实现项目文件保存）
	onExportMap();
}

void MainWindow::OnNewMap()
{
	m_ctx->documentManager.newDefaultDocument();
	if (ui->label)
		ui->label->setText(QStringLiteral("New map created"));
	if (ui->mapViewWidget)
	{
		ui->mapViewWidget->clearAllTiles();
		ui->mapViewWidget->updateMap();
	}

	// 清空 Inspector
	ui->inspectorPanel->clearInfo();
}

// ---------------------  SLOT  ---------------------

void MainWindow::SlotSwitchSpriteSliceWidget()
{
	ui->stackedWidget->setCurrentWidget(ui->page_2);
}

void MainWindow::SlotSwitchMainWidget()
{
	ui->stackedWidget->setCurrentWidget(ui->page);
}