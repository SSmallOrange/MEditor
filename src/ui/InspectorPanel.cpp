#include "InspectorPanel.h"
#include "ui/Common.h"
#include "MapTileItem.h"
#include "core/SpriteSliceDefine.h"

InspectorPanel::InspectorPanel(QWidget* parent)
	: QWidget(parent)
{
	this->setObjectName("inspectorPanel");
	setAttribute(Qt::WA_StyledBackground, true);
	ui.setupUi(this);

	LOAD_QSS(":/InspectorPanel/InspectorPanel.qss");

	initComboBoxes();
	setupConnections();

	// 初始化表格
	ui.tableCustomData->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	ui.tableCustomData->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
	ui.tableCustomData->horizontalHeader()->setMinimumSectionSize(100);
	ui.tableCustomData->horizontalHeader()->setDefaultSectionSize(140);

	// 初始状态：禁用编辑
	setEditEnabled(false);
	clearInfo();
}

void InspectorPanel::initComboBoxes()
{
	// 图集下拉框（动态填充）
	ui.comboTileset->clear();

	// 图层下拉框
	ui.comboLayer->clear();
	ui.comboLayer->addItem(QStringLiteral("图层 0 (背景)"), 0);
	ui.comboLayer->addItem(QStringLiteral("图层 1 (地面)"), 1);
	ui.comboLayer->addItem(QStringLiteral("图层 2 (装饰)"), 2);
	ui.comboLayer->addItem(QStringLiteral("图层 3 (前景)"), 3);

	// 碰撞类型下拉框 - 只保留 None、Ground、Trigger
	ui.comboCollision->clear();
	ui.comboCollision->addItem(QStringLiteral("无碰撞"), static_cast<int>(CollisionType::None));
	ui.comboCollision->addItem(QStringLiteral("地面 (Ground)"), static_cast<int>(CollisionType::Ground));
	ui.comboCollision->addItem(QStringLiteral("触发器 (Trigger)"), static_cast<int>(CollisionType::Trigger));
}

void InspectorPanel::setupConnections()
{
	// 位置变化
	connect(ui.spinPosX, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
		if (m_updating || !m_currentTile) return;
		emit positionChanged(value, ui.spinPosY->value());
	});

	connect(ui.spinPosY, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
		if (m_updating || !m_currentTile) return;
		emit positionChanged(ui.spinPosX->value(), value);
	});

	// 图层变化
	connect(ui.comboLayer, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
		if (m_updating || !m_currentTile) return;
		int layer = ui.comboLayer->itemData(index).toInt();
		emit layerChanged(layer);
	});

	// 名称变化
	connect(ui.editTitle, &QLineEdit::textChanged, this, [this](const QString& text) {
		if (m_updating || !m_currentTile) return;
		emit nameChanged(text);
	});

	// 碰撞类型变化
	connect(ui.comboCollision, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
		if (m_updating || !m_currentTile) return;
		int typeValue = ui.comboCollision->itemData(index).toInt();
		CollisionType type = static_cast<CollisionType>(typeValue);
		emit collisionTypeChanged(type);
	});

	// 标签添加按钮
	connect(ui.buttonAddTag, &QPushButton::clicked, this, &InspectorPanel::onAddTagClicked);

	// 标签列表双击删除（或使用右键菜单）
	connect(ui.listTags, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
		if (m_updating || !m_currentTile) return;
		delete item;
		updateTagsFromList();
	});
}

void InspectorPanel::onAddTagClicked()
{
	if (m_updating || !m_currentTile) return;

	QString newTag = ui.editNewTag->text().trimmed();
	if (newTag.isEmpty()) return;

	// 检查是否已存在
	for (int i = 0; i < ui.listTags->count(); ++i)
	{
		if (ui.listTags->item(i)->text() == newTag)
		{
			ui.editNewTag->clear();
			return;
		}
	}

	// 添加到列表
	ui.listTags->addItem(newTag);
	ui.editNewTag->clear();

	// 更新瓦片标签
	updateTagsFromList();
}

void InspectorPanel::onRemoveTagClicked()
{
	if (m_updating || !m_currentTile) return;

	QListWidgetItem* currentItem = ui.listTags->currentItem();
	if (currentItem)
	{
		delete currentItem;
		updateTagsFromList();
	}
}

void InspectorPanel::updateTagsFromList()
{
	QStringList tagList;
	for (int i = 0; i < ui.listTags->count(); ++i)
	{
		tagList.append(ui.listTags->item(i)->text());
	}
	QString tags = tagList.join(",");
	emit tagsChanged(tags);
}

void InspectorPanel::blockAllSignals(bool block)
{
	ui.spinPosX->blockSignals(block);
	ui.spinPosY->blockSignals(block);
	ui.spinWidth->blockSignals(block);
	ui.spinHeight->blockSignals(block);
	ui.comboTileset->blockSignals(block);
	ui.comboLayer->blockSignals(block);
	ui.comboCollision->blockSignals(block);
	ui.editTileId->blockSignals(block);
	ui.editTitle->blockSignals(block);
	ui.checkLayerVisible->blockSignals(block);
	ui.checkLayerLocked->blockSignals(block);
	ui.listTags->blockSignals(block);
	ui.buttonAddTag->blockSignals(block);
}

void InspectorPanel::showTileInfo(MapTileItem* tile)
{
	if (!tile)
	{
		clearInfo();
		return;
	}

	m_currentTile = tile;
	m_updating = true;
	blockAllSignals(true);

	const SpriteSlice& slice = tile->slice();

	// ========== 选中对象信息 ==========
	ui.labelSelectionType->setText(QStringLiteral("瓦片"));
	ui.labelSelectedPos->setText(QString("(%1, %2)").arg(tile->gridX()).arg(tile->gridY()));

	// ========== 变换信息 ==========
	ui.spinPosX->setValue(tile->gridX());
	ui.spinPosY->setValue(tile->gridY());
	ui.spinWidth->setValue(tile->gridWidth());
	ui.spinHeight->setValue(tile->gridHeight());

	// ========== 图集与瓦片信息 ==========
	// 设置图集下拉框
	QString tilesetId = tile->tilesetId();
	int tilesetIndex = ui.comboTileset->findText(tilesetId);
	if (tilesetIndex < 0)
	{
		ui.comboTileset->addItem(tilesetId);
		tilesetIndex = ui.comboTileset->count() - 1;
	}
	ui.comboTileset->setCurrentIndex(tilesetIndex);

	// 瓦片 ID
	ui.editTileId->setText(slice.id.toString(QUuid::WithoutBraces).left(8));

	// 瓦片名称（使用可编辑的 displayName）
	ui.editTitle->setText(tile->displayName());

	// ========== 图层信息 ==========
	int layerIndex = ui.comboLayer->findData(tile->layer());
	if (layerIndex >= 0)
	{
		ui.comboLayer->setCurrentIndex(layerIndex);
	}

	// ========== 碰撞设置 ==========
	int collisionIndex = ui.comboCollision->findData(static_cast<int>(tile->collisionType()));
	if (collisionIndex >= 0)
	{
		ui.comboCollision->setCurrentIndex(collisionIndex);
	}

	// ========== 标签 ==========
	ui.listTags->clear();
	QString tags = tile->tags();
	if (!tags.isEmpty())
	{
		QStringList tagList = tags.split(',', Qt::SkipEmptyParts);
		for (const QString& tag : tagList)
		{
			ui.listTags->addItem(tag.trimmed());
		}
	}

	blockAllSignals(false);
	m_updating = false;

	setEditEnabled(true);
}

void InspectorPanel::clearInfo()
{
	m_currentTile = nullptr;
	m_updating = true;
	blockAllSignals(true);

	// 选中对象
	ui.labelSelectionType->setText(QStringLiteral("无"));
	ui.labelHoverPos->setText(QStringLiteral("(-,-)"));
	ui.labelSelectedPos->setText(QStringLiteral("(-,-)"));

	// 变换
	ui.spinPosX->setValue(0);
	ui.spinPosY->setValue(0);
	ui.spinWidth->setValue(1);
	ui.spinHeight->setValue(1);

	// 图集与瓦片
	ui.comboTileset->setCurrentIndex(-1);
	ui.editTileId->clear();
	ui.editTitle->clear();

	// 图层
	ui.comboLayer->setCurrentIndex(0);
	ui.checkLayerVisible->setChecked(true);
	ui.checkLayerLocked->setChecked(false);

	// 碰撞
	ui.comboCollision->setCurrentIndex(0);

	// 标签
	ui.listTags->clear();
	ui.editNewTag->clear();

	// 自定义数据
	ui.tableCustomData->clearContents();

	blockAllSignals(false);
	m_updating = false;

	setEditEnabled(false);
}

void InspectorPanel::setEditEnabled(bool enabled)
{
	ui.spinPosX->setEnabled(enabled);
	ui.spinPosY->setEnabled(enabled);
	ui.spinWidth->setEnabled(false);  // 尺寸不可编辑
	ui.spinHeight->setEnabled(false);
	ui.comboTileset->setEnabled(false);  // 图集不可更改
	ui.editTileId->setEnabled(false);  // ID 不可编辑
	ui.editTitle->setEnabled(enabled);
	ui.comboLayer->setEnabled(enabled);
	ui.checkLayerVisible->setEnabled(enabled);
	ui.checkLayerLocked->setEnabled(enabled);
	ui.comboCollision->setEnabled(enabled);
	ui.editNewTag->setEnabled(enabled);
	ui.buttonAddTag->setEnabled(enabled);
	ui.listTags->setEnabled(enabled);
	ui.tableCustomData->setEnabled(enabled);
}