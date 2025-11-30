#pragma once

#include <QMainWindow>
#include "ui_MainWindow.h"

class AppContext;
class MapViewWidget;
class QLabel;
class TitleBarWidget;
class InspectorPanel;

class MainWindow : public QMainWindow
{
public:
	explicit MainWindow(AppContext* ctx, QWidget* parent = nullptr);

private:
	void SetupUi();
	void SetupConnections();
	void SetupStatusBar();

	void OnNewMap();

private:
	AppContext* m_ctx = nullptr;
	
	Ui::MainWindow ui;
};
