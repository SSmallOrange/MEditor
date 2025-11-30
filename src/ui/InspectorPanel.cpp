#include "InspectorPanel.h"

InspectorPanel::InspectorPanel(QWidget* parent)
	: QWidget(parent)
{
	setAttribute(Qt::WA_StyledBackground, true);
	ui.setupUi(this);

	ui.comboTileset->addItem("environment");
	ui.comboTileset->addItem("structures");

	ui.comboCollision->addItem("none");
	ui.comboCollision->addItem("collidable");
	ui.comboCollision->addItem("one-way");

	ui.buttonMore->setFlat(true);
	ui.buttonAddTag->setFlat(true);

	ui.tableCustomData->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	ui.tableCustomData->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
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
