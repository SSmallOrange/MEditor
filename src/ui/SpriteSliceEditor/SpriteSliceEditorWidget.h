#ifndef SPRITESLICEEDITORWIDGET_H
#define SPRITESLICEEDITORWIDGET_H
#include "core/SpriteSliceDefine.h"
#include "SpriteSliceGraphicsView.h"

#include <QWidget>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>

namespace Ui {
	class SpriteSliceEditorWidget;
}

class SpriteSliceEditorWidget : public QWidget
{
	Q_OBJECT

public:
	explicit SpriteSliceEditorWidget(QWidget* parent = nullptr);
	~SpriteSliceEditorWidget();

	void initSliceTable();

	// 导出用：从表格获取所有切片数据
	QVector<SpriteSlice> getAllSlicesFromTable() const;
	SpriteSlice getSliceFromTable(int row) const;

protected:
	bool eventFilter(QObject* watched, QEvent* event) override;

signals:
	void SignalReturnToMainPanel();

private slots:
	// 精灵图管理
	void onAddSheetClicked();
	void onRemoveSheetClicked();
	void onSheetSelectionChanged();

	// 网格设置
	void onGridSettingsChanged();
	void onZoomSliderChanged(int value);
	void onGenerateGridSlicesClicked();
	void onClearGridSlicesClicked();

	// 表格选择变化
	void onSliceSelectionChanged();

	// 右侧面板操作
	void onApplySliceClicked();
	void onResetSliceClicked();
	void onAnchorPresetChanged(int index);
	void onAnchorValueChanged();

	// 模式切换
	void onManualModeToggled(bool checked);
	void onGridModeToggled(bool checked);

	// 手动框选完成
	void onManualSelectionFinished(const QRectF& rect);

	// 鼠标位置更新
	void onMousePosChanged(const QPointF& scenePos);

	// 点击画布选中切片
	void onGraphicsViewClicked(const QPointF& scenePos);

private:
	void setupConnections();
	void loadSpriteSheet(const QString& filePath);
	void displaySpriteSheet(const QPixmap& pixmap);
	void updateGridOverlay();
	void clearGridOverlay();

	// 切片数据管理
	void addSlice(const SpriteSlice& slice);
	void removeSlice(int index);
	void updateSliceInTable(int row, const SpriteSlice& slice);
	void syncTableFromSlices();
	int findSliceIndexById(const QUuid& id) const;

	// 切片查找
	int findSliceAtPoint(const QPointF& point) const;
	QVector<int> findSlicesInRect(const QRectF& rect) const;
	void mergeSlices(const QVector<int>& sliceIndices);

	// 选中切片
	void selectSlice(int sliceIndex);

	// 右侧面板
	void loadSliceToInspector(int sliceIndex);
	void clearInspector();
	void setInspectorEnabled(bool enabled);
	AnchorPreset anchorToPreset(const QPointF& anchor) const;
	QPointF presetToAnchor(AnchorPreset preset) const;

	// 场景高亮
	void updateSelectionHighlight(int sliceIndex);
	void clearSelectionHighlight();

	// 替换 GraphicsView
	void replaceGraphicsView();

private:
	Ui::SpriteSliceEditorWidget* ui;

	// 自定义 GraphicsView
	SpriteSliceGraphicsView* m_graphicsView = nullptr;

	// 场景相关
	QGraphicsScene* m_scene = nullptr;
	QGraphicsPixmapItem* m_spriteSheetItem = nullptr;
	QList<QGraphicsItem*> m_gridLines;

	QGraphicsRectItem* m_highlightRect = nullptr;
	QGraphicsEllipseItem* m_anchorMarker = nullptr;

	// 精灵图数据
	QPixmap m_currentPixmap;
	QString m_currentFilePath;

	// 切片数据（核心数据源）
	QVector<SpriteSlice> m_slices;
	int m_currentSliceIndex = -1;

	// 标记是否正在程序化更新控件
	bool m_updatingInspector = false;
	bool m_updatingSelection = false;

	// 窗口拖拽
	bool m_dragging = false;
	QPoint m_dragStartGlobal;
	QPoint m_windowStartTopLeft;
};

#endif // SPRITESLICEEDITORWIDGET_H