#pragma once
#include "core/SpriteSliceDefine.h"

#include <QMainWindow>

class AppContext;
class MapTileItem;

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
	void SetupInspectorConnections();
	void SetupTitleBarConnections();
	void SetupStatusBar();

	void OnNewMap();
	void InitMapViewUI();

private slots:
	void SlotSwitchSpriteSliceWidget();
	void SlotSwitchMainWidget();

	// MapView 控制槽函数
	void onGridVisibleChanged(bool checked);
	void onGridSizeChanged();
	void onMapSizeChanged();
	void onZoomSliderChanged(int value);
	void onLayerChanged(int index);

	// 瓦片选中相关
	void onTileSelected(MapTileItem* tile);
	void onTileDeselected();

	// Inspector 属性变化
	void onInspectorPositionChanged(int x, int y);
	void onInspectorLayerChanged(int layer);
	void onInspectorNameChanged(const QString& name);
	void onInspectorCollisionTypeChanged(CollisionType type);
	void onInspectorTagsChanged(const QString& tags);

	// 标题栏按钮
	void onExportMap();
	void onResetMap();
	void onSaveMap();

private:
	AppContext* m_ctx = nullptr;

	Ui::MainWindow* ui;
};