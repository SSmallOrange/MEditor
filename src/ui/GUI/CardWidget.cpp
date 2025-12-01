#include "CardWidget.h"
#include <QGraphicsDropShadowEffect>

CardWidget::CardWidget(QWidget* parent)
	: QWidget(parent)
{
	setAttribute(Qt::WA_StyledBackground, true);
	setAutoFillBackground(false);

	// 阴影：让卡片浮起来
	auto shadow = new QGraphicsDropShadowEffect(this);
	shadow->setBlurRadius(12);                 // 阴影柔和程度
	shadow->setOffset(0, 1);                   // 阴影往下偏移
	shadow->setColor(QColor(0, 0, 0, 60));    // 透明度自己调
	setGraphicsEffect(shadow);
}
