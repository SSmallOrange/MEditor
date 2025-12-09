#pragma once
#include <QUuid>
#include <QPoint>
#include <QPixmap>

// ============== 切片数据结构 ==============
struct SpriteSlice
{
	QUuid id;							// 唯一标识符
	QString name;						// 切片名称
	int x = 0;							// X 坐标（Qt坐标系，左上角原点）
	int y = 0;							// Y 坐标（Qt坐标系，左上角原点）
	int width = 32;						// 宽度
	int height = 32;					// 高度
	QString group = "Tiles";			// 分组
	QString tags;						// 标签（逗号分隔）
	bool isCollision = false;			// 是否碰撞瓦片
	bool isDecorationOnly = false;		// 是否仅装饰
	QPointF anchor = { 0.5, 0.5 };		// 锚点（归一化坐标 0~1）

	// 计算锚点的像素坐标
	QPointF anchorPixelPos() const {
		return QPointF(x + width * anchor.x(), y + height * anchor.y());
	}

	// 转换为纹理坐标系的 Y 值（左下角为原点）
	// imageHeight: 整张图片的高度
	int textureY(int imageHeight) const {
		return imageHeight - y - height;
	}
};

// ============== 锚点预设枚举 ==============
enum class AnchorPreset
{
	TopLeft = 0,
	TopCenter,
	TopRight,
	MiddleLeft,
	Center,
	MiddleRight,
	BottomLeft,
	BottomCenter,
	BottomRight,
	Custom
};

namespace SliceTableRole
{
	// 基础 Role 偏移
	constexpr int Base = Qt::UserRole;

	// 切片唯一标识符 (QUuid)
	constexpr int SliceId = Base + 1;

	// 切片名称 (QString) - 也显示在列 1
	constexpr int Name = Base + 2;

	// 位置和尺寸 (int)
	constexpr int PosX = Base + 3;
	constexpr int PosY = Base + 4;
	constexpr int Width = Base + 5;
	constexpr int Height = Base + 6;

	// 分组 (QString)
	constexpr int Group = Base + 7;

	// 标签 (QString)
	constexpr int Tags = Base + 8;

	// 是否碰撞瓦片 (bool)
	constexpr int IsCollision = Base + 9;

	// 是否仅装饰 (bool)
	constexpr int IsDecorationOnly = Base + 10;

	// 锚点 X (double, 0.0~1.0)
	constexpr int AnchorX = Base + 11;

	// 锚点 Y (double, 0.0~1.0)
	constexpr int AnchorY = Base + 12;

	// 精灵图高度
	constexpr int ImageHeight = Base + 13;
}

// 表格列索引
namespace SliceTableColumn
{
	constexpr int Preview = 0;    // 预览图
	constexpr int Name = 1;       // 名称
	constexpr int X = 2;          // X 坐标
	constexpr int Y = 3;          // Y 坐标
	constexpr int Width = 4;      // 宽度
	constexpr int Height = 5;     // 高度
	constexpr int Group = 6;      // 分组
	constexpr int Tags = 7;       // 标签

	constexpr int Count = 8;      // 总列数
}

// ============== 精灵图集导出数据 ==============
struct SpriteSheetData
{
	QString filePath;              // 精灵图文件路径
	QString fileName;              // 精灵图文件名
	QPixmap pixmap;                // 精灵图像素数据
	int imageWidth = 0;            // 图片宽度
	int imageHeight = 0;           // 图片高度
	QVector<SpriteSlice> slices;   // 所有切片数据

	bool isValid() const {
		return !filePath.isEmpty() && !pixmap.isNull() && !slices.isEmpty();
	}

	int sliceCount() const {
		return slices.size();
	}

	// 获取转换为纹理坐标系后的切片 Y 值
	int getTextureY(int sliceIndex) const {
		if (sliceIndex < 0 || sliceIndex >= slices.size())
			return 0;
		return slices[sliceIndex].textureY(imageHeight);
	}
};
