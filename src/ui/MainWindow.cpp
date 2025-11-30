#include "MainWindow.h"
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
	, m_ctx(ctx)
{
	ui.setupUi(this);

	// 无边框窗口（去掉系统标题栏）
	setWindowFlag(Qt::FramelessWindowHint, true);
	setAttribute(Qt::WA_TranslucentBackground, true);

	SetupUi();
	SetupConnections();

	// 默认新建一张地图
	m_ctx->documentManager.newDefaultDocument();
	ui.mapViewWidget->update();
}

void MainWindow::SetupUi()
{
	ui.mapViewWidget->SetContext(m_ctx);
}

void MainWindow::SetupStatusBar()
{
}

void MainWindow::SetupConnections()
{
	QObject::connect(&m_ctx->documentManager, &DocumentManager::documentChanged,
		ui.mapViewWidget, QOverload<>::of(&MapViewWidget::update));

	auto newShortcut = new QShortcut(QKeySequence::New, this);
	QObject::connect(newShortcut, &QShortcut::activated, this, [this]() {
		OnNewMap();
		});
}

void MainWindow::OnNewMap()
{
	m_ctx->documentManager.newDefaultDocument();
	if (ui.label)
		ui.label->setText(QStringLiteral("New map created"));
	if (ui.mapViewWidget)
		ui.mapViewWidget->update();
}

