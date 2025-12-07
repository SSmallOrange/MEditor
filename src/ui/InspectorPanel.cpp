#include "InspectorPanel.h"
#include "ui/Common.h"

InspectorPanel::InspectorPanel(QWidget* parent)
	: QWidget(parent)
{
	this->setObjectName("inspectorPanel");
	setAttribute(Qt::WA_StyledBackground, true);
	ui.setupUi(this);

	LOAD_QSS(":/InspectorPanel/InspectorPanel.qss");

	ui.comboTileset->addItem("environment");
	ui.comboTileset->addItem("structures");

	ui.comboCollision->addItem("none");
	ui.comboCollision->addItem("collidable");
	ui.comboCollision->addItem("one-way");

	ui.tableCustomData->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	ui.tableCustomData->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

	ui.tableCustomData->horizontalHeader()->setMinimumSectionSize(100);
	ui.tableCustomData->horizontalHeader()->setDefaultSectionSize(140);
}

void InspectorPanel::SetTitle(const QString& title)
{
	ui.editTitle->setText(title);
}

void InspectorPanel::SetPosition(int x, int y)
{
	ui.spinPosX->setValue(x);
	ui.spinPosY->setValue(y);
}
