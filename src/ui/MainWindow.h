#pragma once
#include <QMainWindow>

class AppContext;

QT_BEGIN_NAMESPACE
namespace Ui {
	class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
public:
	explicit MainWindow(AppContext* ctx, QWidget* parent = nullptr);

private:
	void SetupUi();
	void SetupConnections();
	void SetupStatusBar();

	void OnNewMap();

private slots:
	void SlotSwitchSpriteSliceWidget();
	void SlotSwitchMainWidget();

private:
	AppContext* m_ctx = nullptr;
	
	Ui::MainWindow* ui;
};
