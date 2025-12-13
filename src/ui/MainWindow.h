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
	Q_OBJECT
public:
	explicit MainWindow(AppContext* ctx, QWidget* parent = nullptr);

private:
	void SetupUi();
	void SetupConnections();
	void SetupMapViewConnections();
	void SetupStatusBar();

	void OnNewMap();
	void InitMapViewUI();  // 初始化 UI 控件显示值

private slots:
	void SlotSwitchSpriteSliceWidget();
	void SlotSwitchMainWidget();

	// MapView 控制槽函数（UI -> MapViewWidget）
	void onGridVisibleChanged(bool checked);
	void onGridSizeChanged();
	void onMapSizeChanged();
	void onZoomSliderChanged(int value);
	void onLayerChanged(int index);

private:
	AppContext* m_ctx = nullptr;

	Ui::MainWindow* ui;
};