#pragma once

#include <QMimeData>
#include <QPixmap>
#include "SpriteSliceDefine.h"

// 拖拽瓦片的 MIME 类型
inline const char* TILE_MIME_TYPE = "application/x-maptile";

// 瓦片拖拽数据
struct TileDragData
{
	QString tilesetId;      // 所属图集 ID
	int sliceIndex = -1;    // 切片索引
	SpriteSlice slice;      // 切片数据
	QPixmap pixmap;         // 原始精灵图（未缩放）

	bool isValid() const {
		return sliceIndex >= 0 && !pixmap.isNull();
	}
};

// 自定义 MimeData 用于拖拽
class TileMimeData : public QMimeData
{
	Q_OBJECT
public:
	explicit TileMimeData(const TileDragData& data)
		: m_data(data)
	{
		// 设置格式标识
		setData(TILE_MIME_TYPE, QByteArray());
	}

	const TileDragData& tileData() const { return m_data; }

	bool hasFormat(const QString& mimeType) const override {
		return mimeType == TILE_MIME_TYPE || QMimeData::hasFormat(mimeType);
	}

	QStringList formats() const override {
		QStringList list = QMimeData::formats();
		list << TILE_MIME_TYPE;
		return list;
	}

private:
	TileDragData m_data;
};