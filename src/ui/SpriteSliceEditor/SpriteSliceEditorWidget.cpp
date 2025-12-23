#include "SpriteSliceEditorWidget.h"
#include "ui_SpriteSliceEditorWidget.h"
#include "ui/Common.h"
#include "core/SpriteSliceDefine.h"

#include <QMouseEvent>
#include <QFileDialog>
#include <QGraphicsLineItem>
#include <QMessageBox>
#include <QActionGroup>
#include <algorithm>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

SpriteSliceEditorWidget::SpriteSliceEditorWidget(QWidget* parent)
	: QWidget(parent)
	, ui(new Ui::SpriteSliceEditorWidget)
{
	setAttribute(Qt::WA_StyledBackground, true);
	ui->setupUi(this);
	LOAD_QSS(":/SpriteSliceEditor/SpriteSliceEditor.qss");

	// 替换默认的 QGraphicsView 为自定义的 SpriteSliceGraphicsView
	replaceGraphicsView();

	// 初始化 Graphics Scene
	m_scene = new QGraphicsScene(this);
	m_graphicsView->setScene(m_scene);
	m_graphicsView->setBackgroundBrush(QBrush(QColor(45, 45, 48)));
	m_graphicsView->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

	// 创建高亮显示项（初始隐藏）
	m_highlightRect = new QGraphicsRectItem();
	m_highlightRect->setPen(QPen(QColor(255, 200, 50), 2));
	m_highlightRect->setBrush(QBrush(QColor(255, 200, 50, 40)));
	m_highlightRect->setZValue(100);
	m_highlightRect->setVisible(false);
	m_scene->addItem(m_highlightRect);

	m_anchorMarker = new QGraphicsEllipseItem();
	m_anchorMarker->setPen(QPen(QColor(255, 50, 50), 2));
	m_anchorMarker->setBrush(QBrush(QColor(255, 50, 50, 180)));
	m_anchorMarker->setZValue(101);
	m_anchorMarker->setVisible(false);
	m_scene->addItem(m_anchorMarker);

	initSliceTable();
	setupConnections();

	qDebug() << "The Object Name: [ " << objectName() << " ]\n";

	// slice 表格列宽自适应
	ui->sliceTableWidget->horizontalHeader()->setStretchLastSection(true);
	ui->sliceTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	ui->sliceTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui->sliceTableWidget->setSelectionMode(QAbstractItemView::SingleSelection);

	// 预填一些 Group 下拉可选项
	ui->groupComboBox->addItem(QStringLiteral("Tiles"));
	ui->groupComboBox->addItem(QStringLiteral("Props"));
	ui->groupComboBox->addItem(QStringLiteral("Characters"));
	ui->groupComboBox->addItem(QStringLiteral("UI"));

	// 初始化锚点预设下拉框
	ui->pivotPresetComboBox->clear();
	ui->pivotPresetComboBox->addItem(QStringLiteral("左上"), static_cast<int>(AnchorPreset::TopLeft));
	ui->pivotPresetComboBox->addItem(QStringLiteral("中上"), static_cast<int>(AnchorPreset::TopCenter));
	ui->pivotPresetComboBox->addItem(QStringLiteral("右上"), static_cast<int>(AnchorPreset::TopRight));
	ui->pivotPresetComboBox->addItem(QStringLiteral("左中"), static_cast<int>(AnchorPreset::MiddleLeft));
	ui->pivotPresetComboBox->addItem(QStringLiteral("居中"), static_cast<int>(AnchorPreset::Center));
	ui->pivotPresetComboBox->addItem(QStringLiteral("右中"), static_cast<int>(AnchorPreset::MiddleRight));
	ui->pivotPresetComboBox->addItem(QStringLiteral("左下"), static_cast<int>(AnchorPreset::BottomLeft));
	ui->pivotPresetComboBox->addItem(QStringLiteral("中下"), static_cast<int>(AnchorPreset::BottomCenter));
	ui->pivotPresetComboBox->addItem(QStringLiteral("右下"), static_cast<int>(AnchorPreset::BottomRight));
	ui->pivotPresetComboBox->addItem(QStringLiteral("自定义"), static_cast<int>(AnchorPreset::Custom));

	// 设置锚点 SpinBox 范围
	ui->pivotXSpinBox->setRange(0.0, 1.0);
	ui->pivotXSpinBox->setSingleStep(0.1);
	ui->pivotYSpinBox->setRange(0.0, 1.0);
	ui->pivotYSpinBox->setSingleStep(0.1);

	// 设置模式按钮为互斥
	auto* modeGroup = new QActionGroup(this);
	modeGroup->addAction(ui->actionGridMode);
	modeGroup->addAction(ui->actionManualMode);
	modeGroup->setExclusive(true);

	// 默认选中 Grid 模式并显示网格
	ui->actionGridMode->setChecked(true);
	ui->actionManualMode->setChecked(false);
	ui->actionToggleGrid->setChecked(true);

	// 初始禁用右侧面板
	setInspectorEnabled(false);

	connect(ui->btnReturn, &QPushButton::clicked, this, &SpriteSliceEditorWidget::SignalReturnToMainPanel);

	QWidget* header = this->findChild<QWidget*>("headerBarWidget");
	if (header)
	{
		header->setMouseTracking(true);
		header->installEventFilter(this);
		header->setCursor(Qt::ArrowCursor);
	}
}

SpriteSliceEditorWidget::~SpriteSliceEditorWidget()
{
	delete ui;
}

void SpriteSliceEditorWidget::replaceGraphicsView()
{
	m_graphicsView = ui->graphicsView;
}

// ============== connect ==============

void SpriteSliceEditorWidget::setupConnections()
{
	// 添加/移除精灵图
	connect(ui->addSheetButton, &QPushButton::clicked, this, &SpriteSliceEditorWidget::onAddSheetClicked);
	connect(ui->removeSheetButton, &QPushButton::clicked, this, &SpriteSliceEditorWidget::onRemoveSheetClicked);
	connect(ui->spriteSheetListWidget, &QListWidget::currentRowChanged, this, &SpriteSliceEditorWidget::onSheetSelectionChanged);

	// 网格设置变化
	connect(ui->tileWidthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &SpriteSliceEditorWidget::onGridSettingsChanged);
	connect(ui->tileHeightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &SpriteSliceEditorWidget::onGridSettingsChanged);
	connect(ui->marginXSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &SpriteSliceEditorWidget::onGridSettingsChanged);
	connect(ui->marginYSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &SpriteSliceEditorWidget::onGridSettingsChanged);
	connect(ui->spacingXSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &SpriteSliceEditorWidget::onGridSettingsChanged);
	connect(ui->spacingYSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &SpriteSliceEditorWidget::onGridSettingsChanged);

	// 缩放滑块
	connect(ui->zoomSlider, &QSlider::valueChanged, this, &SpriteSliceEditorWidget::onZoomSliderChanged);

	// 显示/隐藏网格
	connect(ui->actionToggleGrid, &QAction::toggled, this, [this](bool checked) {
		for (auto* item : m_gridLines) {
			item->setVisible(checked);
		}
		});

	// 生成/清除网格切片
	connect(ui->generateGridSlicesButton, &QPushButton::clicked, this, &SpriteSliceEditorWidget::onGenerateGridSlicesClicked);
	connect(ui->clearGridSlicesButton, &QPushButton::clicked, this, &SpriteSliceEditorWidget::onClearGridSlicesClicked);

	// 工具栏打开图集动作
	connect(ui->actionOpenSheet, &QAction::triggered, this, &SpriteSliceEditorWidget::onAddSheetClicked);

	// 表格选择变化 → 刷新右侧面板
	connect(ui->sliceTableWidget, &QTableWidget::itemSelectionChanged, this, &SpriteSliceEditorWidget::onSliceSelectionChanged);

	// 右侧面板按钮
	connect(ui->applySliceButton, &QPushButton::clicked, this, &SpriteSliceEditorWidget::onApplySliceClicked);
	connect(ui->resetSliceButton, &QPushButton::clicked, this, &SpriteSliceEditorWidget::onResetSliceClicked);

	// 锚点预设变化
	connect(ui->pivotPresetComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SpriteSliceEditorWidget::onAnchorPresetChanged);

	// 锚点数值变化（检测是否变为自定义）
	connect(ui->pivotXSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SpriteSliceEditorWidget::onAnchorValueChanged);
	connect(ui->pivotYSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SpriteSliceEditorWidget::onAnchorValueChanged);

	// 模式切换
	connect(ui->actionManualMode, &QAction::toggled, this, &SpriteSliceEditorWidget::onManualModeToggled);
	connect(ui->actionGridMode, &QAction::toggled, this, &SpriteSliceEditorWidget::onGridModeToggled);

	// 手动框选完成
	connect(m_graphicsView, &SpriteSliceGraphicsView::selectionRectFinished, this, &SpriteSliceEditorWidget::onManualSelectionFinished);

	// 鼠标位置更新
	connect(m_graphicsView, &SpriteSliceGraphicsView::mousePosChanged, this, &SpriteSliceEditorWidget::onMousePosChanged);

	// 点击画布选中切片
	connect(m_graphicsView, &SpriteSliceGraphicsView::clickedAt, this, &SpriteSliceEditorWidget::onGraphicsViewClicked);

	// 确认按钮
	connect(ui->okButton, &QPushButton::clicked, this, &SpriteSliceEditorWidget::onOkButtonClicked);

	// 保存/加载配置 - 连接 toolbar actions
	connect(ui->actionSaveConfig, &QAction::triggered, this, &SpriteSliceEditorWidget::onSaveConfigClicked);
	// connect(ui->actionLoadConfig, &QAction::triggered, this, &SpriteSliceEditorWidget::onLoadConfigClicked);
}

// ============== 确认按钮 ==============
void SpriteSliceEditorWidget::onOkButtonClicked()
{
	// 获取完整的精灵图集数据
	SpriteSheetData data = getSpriteSheetData();

	// 验证数据有效性
	if (!data.isValid())
	{
		QMessageBox::warning(this, QStringLiteral("警告"),
			QStringLiteral("无法导出：请确保已加载精灵图并生成切片"));
		return;
	}

	qDebug() << "=== SpriteSheet Export ===";
	qDebug() << "File:" << data.fileName;
	qDebug() << "Path:" << data.filePath;
	qDebug() << "Image Size:" << data.imageWidth << "x" << data.imageHeight;
	qDebug() << "Slice Count:" << data.sliceCount();

	for (int i = 0; i < data.slices.size(); ++i)
	{
		const auto& slice = data.slices[i];
		int texY = slice.textureY(data.imageHeight);
		qDebug() << QString("  [%1] %2: (%3,%4) %5x%6 group=%7 tags=%8 collision=%9 anchor=(%10,%11)")
			.arg(i)
			.arg(slice.name)
			.arg(slice.x).arg(texY)
			.arg(slice.width).arg(slice.height)
			.arg(slice.group)
			.arg(slice.tags)
			.arg(slice.isCollision)
			.arg(slice.anchor.x()).arg(slice.anchor.y());
	}

	// 发送信号
	emit SignalSpriteSheetConfirmed(data);

	QMessageBox::information(this, QStringLiteral("成功"),
		QStringLiteral("已导出 %1 个切片").arg(data.sliceCount()));
}

SpriteSheetData SpriteSliceEditorWidget::getSpriteSheetData() const
{
	SpriteSheetData data;

	// 填充精灵图信息
	data.filePath = m_currentFilePath;
	data.fileName = QFileInfo(m_currentFilePath).fileName();
	data.pixmap = m_currentPixmap;
	data.imageWidth = m_currentPixmap.width();
	data.imageHeight = m_currentPixmap.height();

	// 从表格获取所有切片数据（确保是最新的）
	data.slices = getAllSlicesFromTable();

	return data;
}

// ============== 模式切换 ==============

void SpriteSliceEditorWidget::onManualModeToggled(bool checked)
{
	m_graphicsView->setManualSelectionEnabled(checked);

	if (checked)
	{
		qDebug() << "Switched to Manual Mode";
	}
}

void SpriteSliceEditorWidget::onGridModeToggled(bool checked)
{
	if (checked)
	{
		m_graphicsView->setManualSelectionEnabled(false);
		qDebug() << "Switched to Grid Mode";
	}
}

void SpriteSliceEditorWidget::onMousePosChanged(const QPointF& scenePos)
{
	ui->mousePosLabel->setText(QString("(%1, %2)").arg(static_cast<int>(scenePos.x())).arg(static_cast<int>(scenePos.y())));
}

void SpriteSliceEditorWidget::onGraphicsViewClicked(const QPointF& scenePos)
{
	int sliceIndex = findSliceAtPoint(scenePos);

	if (sliceIndex >= 0)
	{
		selectSlice(sliceIndex);
	}
}

int SpriteSliceEditorWidget::findSliceAtPoint(const QPointF& point) const
{
	for (int i = m_slices.size() - 1; i >= 0; --i)
	{
		const SpriteSlice& slice = m_slices[i];
		QRectF sliceRect(slice.x, slice.y, slice.width, slice.height);

		if (sliceRect.contains(point))
		{
			return i;
		}
	}

	return -1;
}

void SpriteSliceEditorWidget::selectSlice(int sliceIndex)
{
	if (sliceIndex < 0 || sliceIndex >= m_slices.size())
		return;

	if (m_updatingSelection)
		return;

	m_updatingSelection = true;

	m_currentSliceIndex = sliceIndex;

	ui->sliceTableWidget->blockSignals(true);
	ui->sliceTableWidget->selectRow(sliceIndex);
	ui->sliceTableWidget->blockSignals(false);

	ui->sliceTableWidget->scrollTo(ui->sliceTableWidget->model()->index(sliceIndex, 0));

	loadSliceToInspector(sliceIndex);
	setInspectorEnabled(true);
	updateSelectionHighlight(sliceIndex);

	m_updatingSelection = false;

	qDebug() << "Selected slice:" << m_slices[sliceIndex].name << "at index:" << sliceIndex;
}

// ============== 手动框选处理 ==============

void SpriteSliceEditorWidget::onManualSelectionFinished(const QRectF& rect)
{
	if (m_slices.isEmpty())
	{
		QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先生成网格切片"));
		return;
	}

	QVector<int> selectedIndices = findSlicesInRect(rect);

	if (selectedIndices.size() < 2)
	{
		QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请框选至少两个切片进行合并"));
		return;
	}

	mergeSlices(selectedIndices);
}

QVector<int> SpriteSliceEditorWidget::findSlicesInRect(const QRectF& rect) const
{
	QVector<int> result;

	for (int i = 0; i < m_slices.size(); ++i)
	{
		const SpriteSlice& slice = m_slices[i];
		QRectF sliceRect(slice.x, slice.y, slice.width, slice.height);

		if (rect.intersects(sliceRect))
		{
			QPointF center = sliceRect.center();
			if (rect.contains(center))
			{
				result.append(i);
			}
		}
	}

	return result;
}

void SpriteSliceEditorWidget::mergeSlices(const QVector<int>& sliceIndices)
{
	if (sliceIndices.size() < 2)
		return;

	int minX = INT_MAX, minY = INT_MAX;
	int maxX = INT_MIN, maxY = INT_MIN;

	for (int idx : sliceIndices)
	{
		const SpriteSlice& slice = m_slices[idx];
		minX = qMin(minX, slice.x);
		minY = qMin(minY, slice.y);
		maxX = qMax(maxX, slice.x + slice.width);
		maxY = qMax(maxY, slice.y + slice.height);
	}

	SpriteSlice mergedSlice;
	mergedSlice.id = QUuid::createUuid();
	mergedSlice.name = QString("merged_%1x%2").arg(maxX - minX).arg(maxY - minY);
	mergedSlice.x = minX;
	mergedSlice.y = minY;
	mergedSlice.width = maxX - minX;
	mergedSlice.height = maxY - minY;
	mergedSlice.group = m_slices[sliceIndices.first()].group;
	mergedSlice.tags = "";
	mergedSlice.isCollision = false;
	mergedSlice.isDecorationOnly = false;
	mergedSlice.anchor = QPointF(0.5, 0.5);

	QVector<int> sortedIndices = sliceIndices;
	std::sort(sortedIndices.begin(), sortedIndices.end(), std::greater<int>());

	for (int idx : sortedIndices)
	{
		m_slices.removeAt(idx);
	}

	m_slices.append(mergedSlice);

	syncTableFromSlices();

	int newIndex = m_slices.size() - 1;
	ui->sliceTableWidget->selectRow(newIndex);
	m_currentSliceIndex = newIndex;
	loadSliceToInspector(newIndex);
	setInspectorEnabled(true);
	updateSelectionHighlight(newIndex);

	qDebug() << "Merged" << sliceIndices.size() << "slices into:" << mergedSlice.name;
}

void SpriteSliceEditorWidget::removeSlice(int index)
{
	if (index < 0 || index >= m_slices.size())
		return;

	m_slices.removeAt(index);
	syncTableFromSlices();

	if (m_slices.isEmpty())
	{
		clearInspector();
		setInspectorEnabled(false);
		clearSelectionHighlight();
		m_currentSliceIndex = -1;
	}
	else if (m_currentSliceIndex >= m_slices.size())
	{
		m_currentSliceIndex = m_slices.size() - 1;
		ui->sliceTableWidget->selectRow(m_currentSliceIndex);
	}
}

// ============== 精灵图管理 ==============

void SpriteSliceEditorWidget::onAddSheetClicked()
{
	QString filePath = QFileDialog::getOpenFileName(
		this,
		QStringLiteral("选择图集"),
		QString(),
		QStringLiteral("图片文件 (*.png *.jpg *.jpeg *.bmp *.gif);;JSON 配置 (*.json);;所有文件 (*.*)")
	);

	if (filePath.isEmpty())
		return;

	// 如果选择了 JSON 文件，导入配置
	if (filePath.endsWith(".json", Qt::CaseInsensitive))
	{
		if (!importConfig(filePath))
		{
			QMessageBox::warning(this, QStringLiteral("错误"),
				QStringLiteral("无法加载配置文件"));
		}
		return;
	}

	loadSpriteSheet(filePath);
}

void SpriteSliceEditorWidget::onRemoveSheetClicked()
{
	int currentRow = ui->spriteSheetListWidget->currentRow();
	if (currentRow < 0)
		return;

	delete ui->spriteSheetListWidget->takeItem(currentRow);

	if (ui->spriteSheetListWidget->count() == 0)
	{
		m_scene->clear();
		m_spriteSheetItem = nullptr;
		m_gridLines.clear();
		m_currentPixmap = QPixmap();
		m_currentFilePath.clear();

		m_highlightRect = new QGraphicsRectItem();
		m_highlightRect->setPen(QPen(QColor(255, 200, 50), 2));
		m_highlightRect->setBrush(QBrush(QColor(255, 200, 50, 40)));
		m_highlightRect->setZValue(100);
		m_highlightRect->setVisible(false);
		m_scene->addItem(m_highlightRect);

		m_anchorMarker = new QGraphicsEllipseItem();
		m_anchorMarker->setPen(QPen(QColor(255, 50, 50), 2));
		m_anchorMarker->setBrush(QBrush(QColor(255, 50, 50, 180)));
		m_anchorMarker->setZValue(101);
		m_anchorMarker->setVisible(false);
		m_scene->addItem(m_anchorMarker);

		m_slices.clear();
		ui->sliceTableWidget->setRowCount(0);
		clearInspector();
		setInspectorEnabled(false);
	}
}

void SpriteSliceEditorWidget::onSheetSelectionChanged()
{
	auto* item = ui->spriteSheetListWidget->currentItem();
	if (!item)
		return;

	QString filePath = item->data(Qt::UserRole).toString();
	if (!filePath.isEmpty() && filePath != m_currentFilePath)
	{
		QPixmap pixmap(filePath);
		if (!pixmap.isNull())
		{
			m_currentFilePath = filePath;
			m_currentPixmap = pixmap;
			displaySpriteSheet(pixmap);
		}
	}
}

void SpriteSliceEditorWidget::loadSpriteSheet(const QString& filePath)
{
	QPixmap pixmap(filePath);
	if (pixmap.isNull())
	{
		QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("无法加载图片: %1").arg(filePath));
		return;
	}

	QFileInfo fileInfo(filePath);
	auto* listItem = new QListWidgetItem(fileInfo.fileName());
	listItem->setData(Qt::UserRole, filePath);
	listItem->setToolTip(filePath);
	ui->spriteSheetListWidget->addItem(listItem);
	ui->spriteSheetListWidget->setCurrentItem(listItem);

	m_currentFilePath = filePath;
	m_currentPixmap = pixmap;
	displaySpriteSheet(pixmap);
}

void SpriteSliceEditorWidget::displaySpriteSheet(const QPixmap& pixmap)
{
	auto* tempHighlight = m_highlightRect;
	auto* tempAnchor = m_anchorMarker;

	if (tempHighlight) m_scene->removeItem(tempHighlight);
	if (tempAnchor) m_scene->removeItem(tempAnchor);

	m_scene->clear();
	m_gridLines.clear();
	m_spriteSheetItem = nullptr;

	if (tempHighlight) {
		m_scene->addItem(tempHighlight);
		m_highlightRect = tempHighlight;
	}
	if (tempAnchor) {
		m_scene->addItem(tempAnchor);
		m_anchorMarker = tempAnchor;
	}

	m_spriteSheetItem = m_scene->addPixmap(pixmap);
	m_spriteSheetItem->setPos(0, 0);
	m_spriteSheetItem->setZValue(0);

	m_scene->setSceneRect(pixmap.rect());
	updateGridOverlay();

	m_graphicsView->fitInView(m_spriteSheetItem, Qt::KeepAspectRatio);

	qreal scale = m_graphicsView->transform().m11();
	ui->zoomSlider->blockSignals(true);
	ui->zoomSlider->setValue(static_cast<int>(scale * 100));
	ui->zoomSlider->blockSignals(false);
	ui->zoomLabel->setText(QString("%1%").arg(static_cast<int>(scale * 100)));
}

// ============== 网格相关 ==============

void SpriteSliceEditorWidget::onGridSettingsChanged()
{
	updateGridOverlay();
}

void SpriteSliceEditorWidget::updateGridOverlay()
{
	clearGridOverlay();

	if (m_currentPixmap.isNull() || !ui->actionToggleGrid->isChecked())
		return;

	int tileWidth = ui->tileWidthSpinBox->value();
	int tileHeight = ui->tileHeightSpinBox->value();
	int marginX = ui->marginXSpinBox->value();
	int marginY = ui->marginYSpinBox->value();
	int spacingX = ui->spacingXSpinBox->value();
	int spacingY = ui->spacingYSpinBox->value();

	int imgWidth = m_currentPixmap.width();
	int imgHeight = m_currentPixmap.height();

	QPen gridPen(QColor(255, 100, 100, 180), 1, Qt::SolidLine);

	for (int x = marginX; x <= imgWidth; x += tileWidth + spacingX)
	{
		auto* line = m_scene->addLine(x, 0, x, imgHeight, gridPen);
		line->setZValue(10);
		m_gridLines.append(line);

		if (spacingX > 0 && x + tileWidth <= imgWidth)
		{
			auto* spaceLine = m_scene->addLine(x + tileWidth, 0, x + tileWidth, imgHeight, QPen(QColor(100, 100, 255, 120), 1, Qt::DashLine));
			spaceLine->setZValue(10);
			m_gridLines.append(spaceLine);
		}
	}

	for (int y = marginY; y <= imgHeight; y += tileHeight + spacingY)
	{
		auto* line = m_scene->addLine(0, y, imgWidth, y, gridPen);
		line->setZValue(10);
		m_gridLines.append(line);

		if (spacingY > 0 && y + tileHeight <= imgHeight)
		{
			auto* spaceLine = m_scene->addLine(0, y + tileHeight, imgWidth, y + tileHeight, QPen(QColor(100, 100, 255, 120), 1, Qt::DashLine));
			spaceLine->setZValue(10);
			m_gridLines.append(spaceLine);
		}
	}
}

void SpriteSliceEditorWidget::clearGridOverlay()
{
	for (auto* item : m_gridLines)
	{
		m_scene->removeItem(item);
		delete item;
	}
	m_gridLines.clear();
}

void SpriteSliceEditorWidget::onZoomSliderChanged(int value)
{
	qreal scale = value / 100.0;
	m_graphicsView->resetTransform();
	m_graphicsView->scale(scale, scale);
	ui->zoomLabel->setText(QString("%1%").arg(value));
}

// ============== 切片生成 ==============

void SpriteSliceEditorWidget::onGenerateGridSlicesClicked()
{
	if (m_currentPixmap.isNull())
	{
		QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先加载精灵图"));
		return;
	}

	int tileWidth = ui->tileWidthSpinBox->value();
	int tileHeight = ui->tileHeightSpinBox->value();
	int marginX = ui->marginXSpinBox->value();
	int marginY = ui->marginYSpinBox->value();
	int spacingX = ui->spacingXSpinBox->value();
	int spacingY = ui->spacingYSpinBox->value();
	int startRow = ui->startRowSpinBox->value();
	int startCol = ui->startColumnSpinBox->value();
	int rowCount = ui->rowCountSpinBox->value();
	int colCount = ui->columnCountSpinBox->value();

	int imgWidth = m_currentPixmap.width();
	int imgHeight = m_currentPixmap.height();

	int maxCols = (imgWidth - marginX + spacingX) / (tileWidth + spacingX);
	int maxRows = (imgHeight - marginY + spacingY) / (tileHeight + spacingY);

	if (colCount == 0) colCount = maxCols - startCol;
	if (rowCount == 0) rowCount = maxRows - startRow;

	m_slices.clear();
	ui->sliceTableWidget->setRowCount(0);
	clearInspector();
	setInspectorEnabled(false);
	m_currentSliceIndex = -1;

	for (int row = startRow; row < startRow + rowCount && row < maxRows; ++row)
	{
		for (int col = startCol; col < startCol + colCount && col < maxCols; ++col)
		{
			int x = marginX + col * (tileWidth + spacingX);
			int y = marginY + row * (tileHeight + spacingY);

			SpriteSlice slice;
			slice.id = QUuid::createUuid();
			slice.name = QString("slice_%1_%2").arg(row).arg(col);
			slice.x = x;
			slice.y = y;
			slice.width = tileWidth;
			slice.height = tileHeight;
			slice.group = "Tiles";
			slice.tags = "";
			slice.isCollision = false;
			slice.isDecorationOnly = false;
			slice.anchor = QPointF(0.5, 0.5);

			addSlice(slice);
		}
	}
}

void SpriteSliceEditorWidget::onClearGridSlicesClicked()
{
	m_slices.clear();
	ui->sliceTableWidget->setRowCount(0);
	clearInspector();
	setInspectorEnabled(false);
	clearSelectionHighlight();
	m_currentSliceIndex = -1;
}

// ============== 切片数据管理 ==============

void SpriteSliceEditorWidget::addSlice(const SpriteSlice& slice)
{
	m_slices.append(slice);

	int tableRow = ui->sliceTableWidget->rowCount();
	ui->sliceTableWidget->insertRow(tableRow);

	updateSliceInTable(tableRow, slice);
}

void SpriteSliceEditorWidget::updateSliceInTable(int row, const SpriteSlice& slice)
{
	if (row < 0 || row >= ui->sliceTableWidget->rowCount())
		return;

	QPixmap slicePixmap = m_currentPixmap.copy(slice.x, slice.y, slice.width, slice.height);
	QPixmap thumbnail = slicePixmap.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation);

	auto* previewLabel = qobject_cast<QLabel*>(ui->sliceTableWidget->cellWidget(row, SliceTableColumn::Preview));
	if (!previewLabel) {
		previewLabel = new QLabel;
		previewLabel->setAlignment(Qt::AlignCenter);
		ui->sliceTableWidget->setCellWidget(row, SliceTableColumn::Preview, previewLabel);
	}
	previewLabel->setPixmap(thumbnail);

	auto getOrCreateItem = [this, row](int col) -> QTableWidgetItem* {
		auto* item = ui->sliceTableWidget->item(row, col);
		if (!item) {
			item = new QTableWidgetItem();
			ui->sliceTableWidget->setItem(row, col, item);
		}
		return item;
	};

	int textureY = slice.textureY(m_currentPixmap.height());

	auto* nameItem = getOrCreateItem(SliceTableColumn::Name);
	nameItem->setText(slice.name);
	nameItem->setData(SliceTableRole::SliceId, slice.id);
	nameItem->setData(SliceTableRole::Name, slice.name);
	nameItem->setData(SliceTableRole::PosX, slice.x);
	nameItem->setData(SliceTableRole::PosY, textureY);
	nameItem->setData(SliceTableRole::Width, slice.width);
	nameItem->setData(SliceTableRole::Height, slice.height);
	nameItem->setData(SliceTableRole::Group, slice.group);
	nameItem->setData(SliceTableRole::Tags, slice.tags);
	nameItem->setData(SliceTableRole::IsCollision, slice.isCollision);
	nameItem->setData(SliceTableRole::IsDecorationOnly, slice.isDecorationOnly);
	nameItem->setData(SliceTableRole::AnchorX, slice.anchor.x());
	nameItem->setData(SliceTableRole::AnchorY, slice.anchor.y());
	nameItem->setData(SliceTableRole::ImageHeight, m_currentPixmap.height());
	nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);

	auto setDisplayItem = [&getOrCreateItem](int col, const QString& text, bool center = false) {
		auto* item = getOrCreateItem(col);
		item->setText(text);
		if (center) item->setTextAlignment(Qt::AlignCenter);
		item->setFlags(item->flags() & ~Qt::ItemIsEditable);
	};

	setDisplayItem(SliceTableColumn::X, QString::number(slice.x), true);
	setDisplayItem(SliceTableColumn::Y, QString::number(slice.y), true);
	setDisplayItem(SliceTableColumn::Width, QString::number(slice.width), true);
	setDisplayItem(SliceTableColumn::Height, QString::number(slice.height), true);
	setDisplayItem(SliceTableColumn::Group, slice.group);
	setDisplayItem(SliceTableColumn::Tags, slice.tags);
}

SpriteSlice SpriteSliceEditorWidget::getSliceFromTable(int row) const
{
	SpriteSlice slice;

	if (row < 0 || row >= ui->sliceTableWidget->rowCount())
		return slice;

	auto* nameItem = ui->sliceTableWidget->item(row, SliceTableColumn::Name);
	if (!nameItem)
		return slice;

	slice.id = nameItem->data(SliceTableRole::SliceId).toUuid();
	slice.name = nameItem->data(SliceTableRole::Name).toString();
	slice.x = nameItem->data(SliceTableRole::PosX).toInt();
	slice.y = nameItem->data(SliceTableRole::PosY).toInt();
	slice.width = nameItem->data(SliceTableRole::Width).toInt();
	slice.height = nameItem->data(SliceTableRole::Height).toInt();
	slice.group = nameItem->data(SliceTableRole::Group).toString();
	slice.tags = nameItem->data(SliceTableRole::Tags).toString();
	slice.isCollision = nameItem->data(SliceTableRole::IsCollision).toBool();
	slice.isDecorationOnly = nameItem->data(SliceTableRole::IsDecorationOnly).toBool();
	slice.anchor = QPointF(
		nameItem->data(SliceTableRole::AnchorX).toDouble(),
		nameItem->data(SliceTableRole::AnchorY).toDouble()
	);

	return slice;
}

QVector<SpriteSlice> SpriteSliceEditorWidget::getAllSlicesFromTable() const
{
	QVector<SpriteSlice> slices;
	slices.reserve(ui->sliceTableWidget->rowCount());

	for (int i = 0; i < ui->sliceTableWidget->rowCount(); ++i)
	{
		slices.append(getSliceFromTable(i));
	}

	return slices;
}

void SpriteSliceEditorWidget::syncTableFromSlices()
{
	ui->sliceTableWidget->setRowCount(0);
	for (int i = 0; i < m_slices.size(); ++i)
	{
		ui->sliceTableWidget->insertRow(i);
		updateSliceInTable(i, m_slices[i]);
	}
}

int SpriteSliceEditorWidget::findSliceIndexById(const QUuid& id) const
{
	for (int i = 0; i < m_slices.size(); ++i)
	{
		if (m_slices[i].id == id)
			return i;
	}
	return -1;
}

// ============== 右侧面板 ==============

void SpriteSliceEditorWidget::onSliceSelectionChanged()
{
	if (m_updatingSelection)
		return;

	int currentRow = ui->sliceTableWidget->currentRow();

	if (currentRow < 0 || currentRow >= m_slices.size())
	{
		clearInspector();
		setInspectorEnabled(false);
		clearSelectionHighlight();
		m_currentSliceIndex = -1;
		return;
	}

	m_currentSliceIndex = currentRow;
	loadSliceToInspector(currentRow);
	setInspectorEnabled(true);
	updateSelectionHighlight(currentRow);
}

void SpriteSliceEditorWidget::loadSliceToInspector(int sliceIndex)
{
	if (sliceIndex < 0 || sliceIndex >= m_slices.size())
		return;

	m_updatingInspector = true;

	const SpriteSlice& slice = m_slices[sliceIndex];
	int imageHeight = m_currentPixmap.height();

	ui->sliceNameLineEdit->setText(slice.name);
	ui->sliceIdLineEdit->setText(slice.id.toString(QUuid::WithoutBraces).left(8));
	ui->sliceIndexSpinBox->setValue(sliceIndex);
	ui->sliceXSpinBox->setValue(slice.x);
	ui->sliceYSpinBox->setValue(slice.textureY(imageHeight));
	ui->sliceWidthSpinBox->setValue(slice.width);
	ui->sliceHeightSpinBox->setValue(slice.height);

	int groupIndex = ui->groupComboBox->findText(slice.group);
	if (groupIndex >= 0) {
		ui->groupComboBox->setCurrentIndex(groupIndex);
	}
	else {
		ui->groupComboBox->setCurrentText(slice.group);
	}
	ui->tagsLineEdit->setText(slice.tags);

	ui->collisionCheckBox->setChecked(slice.isCollision);
	ui->decorCheckBox->setChecked(slice.isDecorationOnly);

	AnchorPreset preset = anchorToPreset(slice.anchor);
	int presetIndex = ui->pivotPresetComboBox->findData(static_cast<int>(preset));
	if (presetIndex >= 0) {
		ui->pivotPresetComboBox->setCurrentIndex(presetIndex);
	}
	ui->pivotXSpinBox->setValue(slice.anchor.x());
	ui->pivotYSpinBox->setValue(slice.anchor.y());

	m_updatingInspector = false;
}

void SpriteSliceEditorWidget::clearInspector()
{
	m_updatingInspector = true;

	ui->sliceNameLineEdit->clear();
	ui->sliceIdLineEdit->clear();
	ui->sliceIndexSpinBox->setValue(0);
	ui->sliceXSpinBox->setValue(0);
	ui->sliceYSpinBox->setValue(0);
	ui->sliceWidthSpinBox->setValue(0);
	ui->sliceHeightSpinBox->setValue(0);
	ui->groupComboBox->setCurrentIndex(0);
	ui->tagsLineEdit->clear();
	ui->collisionCheckBox->setChecked(false);
	ui->decorCheckBox->setChecked(false);
	ui->pivotPresetComboBox->setCurrentIndex(static_cast<int>(AnchorPreset::Center));
	ui->pivotXSpinBox->setValue(0.5);
	ui->pivotYSpinBox->setValue(0.5);

	m_updatingInspector = false;
}

void SpriteSliceEditorWidget::setInspectorEnabled(bool enabled)
{
	ui->sliceNameLineEdit->setEnabled(enabled);
	ui->sliceIndexSpinBox->setEnabled(enabled);
	ui->groupComboBox->setEnabled(enabled);
	ui->tagsLineEdit->setEnabled(enabled);
	ui->collisionCheckBox->setEnabled(enabled);
	ui->decorCheckBox->setEnabled(enabled);
	ui->pivotPresetComboBox->setEnabled(enabled);
	ui->pivotXSpinBox->setEnabled(enabled);
	ui->pivotYSpinBox->setEnabled(enabled);
	ui->applySliceButton->setEnabled(enabled);
	ui->resetSliceButton->setEnabled(enabled);
}

void SpriteSliceEditorWidget::onApplySliceClicked()
{
	if (m_currentSliceIndex < 0 || m_currentSliceIndex >= m_slices.size())
		return;

	SpriteSlice& slice = m_slices[m_currentSliceIndex];

	slice.name = ui->sliceNameLineEdit->text();
	slice.group = ui->groupComboBox->currentText();
	slice.tags = ui->tagsLineEdit->text();
	slice.isCollision = ui->collisionCheckBox->isChecked();
	slice.isDecorationOnly = ui->decorCheckBox->isChecked();
	slice.anchor = QPointF(ui->pivotXSpinBox->value(), ui->pivotYSpinBox->value());

	// 同步更新到表格（包括显示和存储的数据）
	updateSliceInTable(m_currentSliceIndex, slice);

	// 更新高亮显示
	updateSelectionHighlight(m_currentSliceIndex);

	qDebug() << "Applied slice:" << slice.name
		<< "group:" << slice.group
		<< "tags:" << slice.tags
		<< "collision:" << slice.isCollision
		<< "decoration:" << slice.isDecorationOnly
		<< "anchor:" << slice.anchor;
}

void SpriteSliceEditorWidget::onResetSliceClicked()
{
	if (m_currentSliceIndex < 0 || m_currentSliceIndex >= m_slices.size())
		return;

	loadSliceToInspector(m_currentSliceIndex);
}

void SpriteSliceEditorWidget::onAnchorPresetChanged(int index)
{
	if (m_updatingInspector)
		return;

	int presetValue = ui->pivotPresetComboBox->itemData(index).toInt();
	AnchorPreset preset = static_cast<AnchorPreset>(presetValue);

	if (preset == AnchorPreset::Custom)
		return;

	QPointF anchor = presetToAnchor(preset);

	m_updatingInspector = true;
	ui->pivotXSpinBox->setValue(anchor.x());
	ui->pivotYSpinBox->setValue(anchor.y());
	m_updatingInspector = false;
}

void SpriteSliceEditorWidget::onAnchorValueChanged()
{
	if (m_updatingInspector)
		return;

	QPointF currentAnchor(ui->pivotXSpinBox->value(), ui->pivotYSpinBox->value());
	AnchorPreset matchedPreset = anchorToPreset(currentAnchor);

	m_updatingInspector = true;
	int presetIndex = ui->pivotPresetComboBox->findData(static_cast<int>(matchedPreset));
	if (presetIndex >= 0) {
		ui->pivotPresetComboBox->setCurrentIndex(presetIndex);
	}
	m_updatingInspector = false;
}

AnchorPreset SpriteSliceEditorWidget::anchorToPreset(const QPointF& anchor) const
{
	const double eps = 0.001;

	auto eq = [eps](double a, double b) { return qAbs(a - b) < eps; };

	if (eq(anchor.x(), 0.0) && eq(anchor.y(), 0.0)) return AnchorPreset::TopLeft;
	if (eq(anchor.x(), 0.5) && eq(anchor.y(), 0.0)) return AnchorPreset::TopCenter;
	if (eq(anchor.x(), 1.0) && eq(anchor.y(), 0.0)) return AnchorPreset::TopRight;
	if (eq(anchor.x(), 0.0) && eq(anchor.y(), 0.5)) return AnchorPreset::MiddleLeft;
	if (eq(anchor.x(), 0.5) && eq(anchor.y(), 0.5)) return AnchorPreset::Center;
	if (eq(anchor.x(), 1.0) && eq(anchor.y(), 0.5)) return AnchorPreset::MiddleRight;
	if (eq(anchor.x(), 0.0) && eq(anchor.y(), 1.0)) return AnchorPreset::BottomLeft;
	if (eq(anchor.x(), 0.5) && eq(anchor.y(), 1.0)) return AnchorPreset::BottomCenter;
	if (eq(anchor.x(), 1.0) && eq(anchor.y(), 1.0)) return AnchorPreset::BottomRight;

	return AnchorPreset::Custom;
}

QPointF SpriteSliceEditorWidget::presetToAnchor(AnchorPreset preset) const
{
	switch (preset)
	{
	case AnchorPreset::TopLeft:      return { 0.0, 0.0 };
	case AnchorPreset::TopCenter:    return { 0.5, 0.0 };
	case AnchorPreset::TopRight:     return { 1.0, 0.0 };
	case AnchorPreset::MiddleLeft:   return { 0.0, 0.5 };
	case AnchorPreset::Center:       return { 0.5, 0.5 };
	case AnchorPreset::MiddleRight:  return { 1.0, 0.5 };
	case AnchorPreset::BottomLeft:   return { 0.0, 1.0 };
	case AnchorPreset::BottomCenter: return { 0.5, 1.0 };
	case AnchorPreset::BottomRight:  return { 1.0, 1.0 };
	default:                         return { 0.5, 0.5 };
	}
}

// ============== 场景高亮 ==============

void SpriteSliceEditorWidget::updateSelectionHighlight(int sliceIndex)
{
	if (sliceIndex < 0 || sliceIndex >= m_slices.size())
	{
		clearSelectionHighlight();
		return;
	}

	const SpriteSlice& slice = m_slices[sliceIndex];

	m_highlightRect->setRect(slice.x, slice.y, slice.width, slice.height);
	m_highlightRect->setVisible(true);

	QPointF anchorPos = slice.anchorPixelPos();
	const double markerRadius = 4.0;
	m_anchorMarker->setRect(
		anchorPos.x() - markerRadius,
		anchorPos.y() - markerRadius,
		markerRadius * 2,
		markerRadius * 2
	);
	m_anchorMarker->setVisible(true);
}

void SpriteSliceEditorWidget::clearSelectionHighlight()
{
	if (m_highlightRect)
		m_highlightRect->setVisible(false);
	if (m_anchorMarker)
		m_anchorMarker->setVisible(false);
}

// ============== 表格初始化 ==============

void SpriteSliceEditorWidget::initSliceTable()
{
	auto* table = ui->sliceTableWidget;
	if (!table) return;

	auto* hv = table->horizontalHeader();
	hv->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	hv->setStretchLastSection(true);
	hv->setMinimumSectionSize(80);
	hv->setHighlightSections(false);

	table->setColumnWidth(SliceTableColumn::Preview, 80);
	table->setColumnWidth(SliceTableColumn::Name, 160);

	table->setRowCount(0);
}

// ============== 事件过滤器 ==============

bool SpriteSliceEditorWidget::eventFilter(QObject* watched, QEvent* event)
{
	QWidget* header = this->findChild<QWidget*>("headerBarWidget");
	if (watched == header)
	{
		switch (event->type())
		{
		case QEvent::MouseButtonPress:
		{
			auto* me = reinterpret_cast<QMouseEvent*>(event);
			if (me->button() == Qt::LeftButton)
			{
				m_dragging = true;
				m_dragStartGlobal = me->globalPosition().toPoint();
				m_windowStartTopLeft = window()->frameGeometry().topLeft();
				return true;
			}
			break;
		}
		case QEvent::MouseMove:
		{
			if (m_dragging)
			{
				auto* me = static_cast<QMouseEvent*>(event);
				const QPoint currentGlobal = me->globalPosition().toPoint();
				const QPoint delta = currentGlobal - m_dragStartGlobal;

				window()->move(m_windowStartTopLeft + delta);
				return true;
			}
			break;
		}
		case QEvent::MouseButtonRelease:
		{
			auto* me = static_cast<QMouseEvent*>(event);
			if (me->button() == Qt::LeftButton)
			{
				m_dragging = false;
				return true;
			}
			break;
		}
		default:
			break;
		}
	}
	return QWidget::eventFilter(watched, event);
}

// ============== 配置导入/导出 ==============

void SpriteSliceEditorWidget::onSaveConfigClicked()
{
	if (m_currentFilePath.isEmpty() || m_slices.isEmpty())
	{
		QMessageBox::warning(this, QStringLiteral("警告"),
			QStringLiteral("请先加载精灵图并创建切片"));
		return;
	}

	QString defaultName = QFileInfo(m_currentFilePath).baseName() + ".json";
	QString jsonPath = QFileDialog::getSaveFileName(
		this,
		QStringLiteral("保存精灵图配置"),
		defaultName,
		QStringLiteral("JSON 文件 (*.json)")
	);

	if (jsonPath.isEmpty())
		return;

	if (exportConfig(jsonPath))
	{
		QMessageBox::information(this, QStringLiteral("成功"),
			QStringLiteral("配置已保存到: %1").arg(jsonPath));
	}
	else
	{
		QMessageBox::critical(this, QStringLiteral("错误"),
			QStringLiteral("保存配置失败"));
	}
}

bool SpriteSliceEditorWidget::exportConfig(const QString& jsonPath)
{
	QJsonObject root;
	root["version"] = "1.0";

	// 精灵图信息
	QJsonObject spriteSheet;
	// 计算相对路径（相对于 JSON 文件所在目录）
	QDir jsonDir = QFileInfo(jsonPath).absoluteDir();
	QString relativePath = jsonDir.relativeFilePath(m_currentFilePath);
	spriteSheet["imagePath"] = relativePath;
	spriteSheet["imageWidth"] = m_currentPixmap.width();
	spriteSheet["imageHeight"] = m_currentPixmap.height();
	root["spriteSheet"] = spriteSheet;

	// 网格设置
	QJsonObject gridSettings;
	gridSettings["tileWidth"] = ui->tileWidthSpinBox->value();
	gridSettings["tileHeight"] = ui->tileHeightSpinBox->value();
	gridSettings["marginX"] = ui->marginXSpinBox->value();
	gridSettings["marginY"] = ui->marginYSpinBox->value();
	gridSettings["spacingX"] = ui->spacingXSpinBox->value();
	gridSettings["spacingY"] = ui->spacingYSpinBox->value();
	root["gridSettings"] = gridSettings;

	// 切片数据
	QJsonArray slicesArray;
	for (const SpriteSlice& slice : m_slices)
	{
		QJsonObject sliceObj;
		sliceObj["id"] = slice.id.toString(QUuid::WithoutBraces);
		sliceObj["name"] = slice.name;
		sliceObj["x"] = slice.x;
		sliceObj["y"] = slice.y;
		sliceObj["width"] = slice.width;
		sliceObj["height"] = slice.height;
		sliceObj["group"] = slice.group;
		sliceObj["tags"] = slice.tags;
		sliceObj["isCollision"] = slice.isCollision;
		sliceObj["isDecorationOnly"] = slice.isDecorationOnly;

		QJsonObject anchorObj;
		anchorObj["x"] = slice.anchor.x();
		anchorObj["y"] = slice.anchor.y();
		sliceObj["anchor"] = anchorObj;

		sliceObj["collisionType"] = static_cast<int>(slice.collisionType);

		slicesArray.append(sliceObj);
	}
	root["slices"] = slicesArray;

	// 写入文件
	QFile file(jsonPath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		qWarning() << "Failed to open file for writing:" << jsonPath;
		return false;
	}

	QJsonDocument doc(root);
	file.write(doc.toJson(QJsonDocument::Indented));
	file.close();

	qDebug() << "Exported config to:" << jsonPath;
	return true;
}

bool SpriteSliceEditorWidget::importConfig(const QString& jsonPath)
{
	QFile file(jsonPath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "Failed to open file for reading:" << jsonPath;
		return false;
	}

	QByteArray data = file.readAll();
	file.close();

	QJsonParseError parseError;
	QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
	if (parseError.error != QJsonParseError::NoError)
	{
		qWarning() << "JSON parse error:" << parseError.errorString();
		return false;
	}

	QJsonObject root = doc.object();

	// 读取精灵图信息
	QJsonObject spriteSheet = root["spriteSheet"].toObject();
	QString imagePath = spriteSheet["imagePath"].toString();

	// 将相对路径转换为绝对路径
	QDir jsonDir = QFileInfo(jsonPath).absoluteDir();
	QString absoluteImagePath = jsonDir.absoluteFilePath(imagePath);

	// 检查图片是否存在
	if (!QFile::exists(absoluteImagePath))
	{
		QMessageBox::warning(this, QStringLiteral("警告"),
			QStringLiteral("找不到精灵图文件: %1\n请确保图片位于正确位置").arg(absoluteImagePath));
		return false;
	}

	// 加载精灵图
	loadSpriteSheet(absoluteImagePath);

	// 读取网格设置
	if (root.contains("gridSettings"))
	{
		QJsonObject gridSettings = root["gridSettings"].toObject();
		ui->tileWidthSpinBox->setValue(gridSettings["tileWidth"].toInt(32));
		ui->tileHeightSpinBox->setValue(gridSettings["tileHeight"].toInt(32));
		ui->marginXSpinBox->setValue(gridSettings["marginX"].toInt(0));
		ui->marginYSpinBox->setValue(gridSettings["marginY"].toInt(0));
		ui->spacingXSpinBox->setValue(gridSettings["spacingX"].toInt(0));
		ui->spacingYSpinBox->setValue(gridSettings["spacingY"].toInt(0));
	}

	// 读取切片数据
	m_slices.clear();
	ui->sliceTableWidget->setRowCount(0);

	QJsonArray slicesArray = root["slices"].toArray();
	for (const QJsonValue& value : slicesArray)
	{
		QJsonObject sliceObj = value.toObject();

		SpriteSlice slice;
		slice.id = QUuid::fromString(sliceObj["id"].toString());
		if (slice.id.isNull())
			slice.id = QUuid::createUuid();

		slice.name = sliceObj["name"].toString();
		slice.x = sliceObj["x"].toInt();
		slice.y = sliceObj["y"].toInt();
		slice.width = sliceObj["width"].toInt();
		slice.height = sliceObj["height"].toInt();
		slice.group = sliceObj["group"].toString("Tiles");
		slice.tags = sliceObj["tags"].toString();
		slice.isCollision = sliceObj["isCollision"].toBool();
		slice.isDecorationOnly = sliceObj["isDecorationOnly"].toBool();

		if (sliceObj.contains("anchor"))
		{
			QJsonObject anchorObj = sliceObj["anchor"].toObject();
			slice.anchor = QPointF(anchorObj["x"].toDouble(0.5), anchorObj["y"].toDouble(0.5));
		}

		slice.collisionType = static_cast<CollisionType>(sliceObj["collisionType"].toInt(0));

		addSlice(slice);
	}

	// 清空检查器
	clearInspector();
	setInspectorEnabled(false);
	clearSelectionHighlight();
	m_currentSliceIndex = -1;

	qDebug() << "Imported config from:" << jsonPath << "with" << m_slices.size() << "slices";
	return true;
}
