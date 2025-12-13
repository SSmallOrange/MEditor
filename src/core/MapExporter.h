#pragma once

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QVector>
#include <QMap>

class MapTileItem;
struct MapDocument;
struct SpriteSlice;

// 地图导出器
class MapExporter
{
public:
	struct ExportOptions
	{
		bool includeEmptyLayers = false;   // 是否导出空图层
		bool prettyPrint = true;           // 是否格式化输出
		int indentSize = 2;                // 缩进大小
	};

	// 导出地图到 JSON 文件
	static bool exportToJson(
		const QString& filePath,
		const MapDocument* document,
		const QVector<MapTileItem*>& tiles,
		int tileWidth,
		int tileHeight,
		const ExportOptions& options = ExportOptions()
	);

	// 获取最后一次错误信息
	static QString lastError() { return s_lastError; }

private:
	// 构建地图元数据
	static QJsonObject buildMapMeta(const MapDocument* doc, int tileWidth, int tileHeight);

	// 构建图层数据
	static QJsonArray buildLayers(
		const QVector<MapTileItem*>& tiles,
		int layerCount,
		const QMap<QString, int>& sliceIdMap
	);

	// 构建单个瓦片数据（使用 sliceId 引用）
	static QJsonObject buildTileData(MapTileItem* tile, const QMap<QString, int>& sliceIdMap);

	// 构建切片数据数组
	static QJsonArray buildSlicesArray(
		const QVector<MapTileItem*>& tiles,
		QMap<QString, int>& outSliceIdMap
	);

	// 构建单个切片数据
	static QJsonObject buildSliceData(const SpriteSlice& slice);

	// 构建图集引用数据
	static QJsonArray buildTilesetReferences(const QVector<MapTileItem*>& tiles);

	static QString s_lastError;
};