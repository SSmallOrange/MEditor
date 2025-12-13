#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "MapViewWidget.h"
#include "TitleBarWidget.h"
#include "InspectorPanel.h"

#include "app/AppContext.h"
#include "app/DocumentManager.h"

#include <QMenuBar>
#include <QStatusBar>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <QApplication>
#include <QShortcut>
#include <QKeySequence>
#include <QGraphicsDropShadowEffect>

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

	connect(ui->TilesetsPanelWidget, &TilesetsPanel::addTilesetRequested, this, &MainWindow::SlotSwitchSpriteSliceWidget);
	connect(ui->spriteSliceEditorWidget, &SpriteSliceEditorWidget::SignalReturnToMainPanel, this, &MainWindow::SlotSwitchMainWidget);
	connect(ui->spriteSliceEditorWidget, &SpriteSliceEditorWidget::SignalSpriteSheetConfirmed, ui->TilesetsPanelWidget, &TilesetsPanel::onSpriteSheetConfirmed);

	// 设置 MapView 控制连接
	SetupMapViewConnections();
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

	// ========== MapViewWidget -> UI 控件（仅用于滚轮缩放时同步显示） ==========
	connect(ui->mapViewWidget, &MapViewWidget::zoomChanged, this, [this](int percent) {
		ui->zoomSlider->blockSignals(true);
		ui->zoomSlider->setValue(percent);
		ui->zoomSlider->blockSignals(false);
		ui->zoomLabel->setText(QString("%1%").arg(percent));
		});
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

void MainWindow::OnNewMap()
{
	m_ctx->documentManager.newDefaultDocument();
	if (ui->label)
		ui->label->setText(QStringLiteral("New map created"));
	if (ui->mapViewWidget)
		ui->mapViewWidget->updateMap();
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