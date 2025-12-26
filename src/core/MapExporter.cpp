#include "MapExporter.h"
#include "MapDocument.h"
#include "SpriteSliceDefine.h"
#include "ui/MapTileItem.h"

#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QFile>
#include <QSet>
#include <QDir>


QString MapExporter::s_lastError;

bool MapExporter::exportToJson(
	const QString& filePath,
	const MapDocument* document,
	const QVector<MapTileItem*>& tiles,
	const QVector<SpriteSheetData>& tilesets,
	int tileWidth,
	int tileHeight,
	const ExportOptions& options)
{
	if (!document)
	{
		s_lastError = "Invalid map document";
		return false;
	}

	QJsonObject root;

	// ========== 文件头信息 ==========
	QJsonObject header;
	header["version"] = "1.3";  // 版本升级
	header["generator"] = "MEditor";
	header["exportTime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
	root["header"] = header;

	// ========== 地图元数据 ==========
	root["map"] = buildMapMeta(document, tileWidth, tileHeight);

	// ========== 图集数据（包含完整精灵图配置） ==========
	root["tilesets"] = buildTilesetData(tilesets, filePath);

	// ========== 切片数据 ==========
	QMap<QString, int> sliceIdMap;  // UUID -> 数组索引
	root["slices"] = buildSlicesArray(tiles, sliceIdMap);

	// ========== 图层数据 ==========
	const int layerCount = 4;
	root["layers"] = buildLayers(tiles, layerCount, sliceIdMap);

	// ========== 写入文件 ==========
	QJsonDocument jsonDoc(root);
	QFile file(filePath);

	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		s_lastError = QString("Cannot open file for writing: %1").arg(file.errorString());
		return false;
	}

	QJsonDocument::JsonFormat format = options.prettyPrint
		? QJsonDocument::Indented
		: QJsonDocument::Compact;

	file.write(jsonDoc.toJson(format));
	file.close();

	s_lastError.clear();
	return true;
}

QJsonArray MapExporter::buildTilesetData(const QVector<SpriteSheetData>& tilesets, const QString& jsonFilePath)
{
	QJsonArray tilesetsArray;
	QDir jsonDir = QFileInfo(jsonFilePath).absoluteDir();

	int index = 0;
	for (const SpriteSheetData& data : tilesets)
	{
		QJsonObject tilesetObj;

		// 基本信息
		tilesetObj["id"] = index++;
		tilesetObj["name"] = data.fileName;

		// 计算相对路径
		QString relativePath = jsonDir.relativeFilePath(data.filePath);
		tilesetObj["imagePath"] = relativePath;
		tilesetObj["imageWidth"] = data.imageWidth;
		tilesetObj["imageHeight"] = data.imageHeight;

		// 切片数据
		QJsonArray slicesArray;
		for (const SpriteSlice& slice : data.slices)
		{
			QJsonObject sliceObj;
			sliceObj["id"] = slice.id.toString(QUuid::WithoutBraces);
			sliceObj["name"] = slice.name;
			sliceObj["x"] = slice.x;
			sliceObj["y"] = slice.y;
			sliceObj["width"] = slice.width;
			sliceObj["height"] = slice.height;
			sliceObj["group"] = slice.group;
			sliceObj["tags"] = slice.tags;
			sliceObj["isCollision"] = slice.isCollision;
			sliceObj["isDecorationOnly"] = slice.isDecorationOnly;

			QJsonObject anchorObj;
			anchorObj["x"] = slice.anchor.x();
			anchorObj["y"] = slice.anchor.y();
			sliceObj["anchor"] = anchorObj;

			sliceObj["collisionType"] = static_cast<int>(slice.collisionType);

			slicesArray.append(sliceObj);
		}
		tilesetObj["slices"] = slicesArray;

		tilesetsArray.append(tilesetObj);
	}

	return tilesetsArray;
}

QJsonObject MapExporter::buildMapMeta(const MapDocument* doc, int tileWidth, int tileHeight)
{
	QJsonObject map;

	// 基本信息
	map["name"] = doc->name.isEmpty() ? "Untitled" : doc->name;
	map["width"] = doc->width;
	map["height"] = doc->height;
	map["tileWidth"] = tileWidth;
	map["tileHeight"] = tileHeight;

	// 像素尺寸
	QJsonObject pixelSize;
	pixelSize["width"] = doc->width * tileWidth;
	pixelSize["height"] = doc->height * tileHeight;
	map["pixelSize"] = pixelSize;

	return map;
}

QJsonArray MapExporter::buildSlicesArray(
	const QVector<MapTileItem*>& tiles,
	QMap<QString, int>& outSliceIdMap)
{
	QJsonArray slices;
	QSet<QString> addedSlices;  // 用于去重

	int index = 0;
	for (const auto* tile : tiles)
	{
		if (!tile)
			continue;

		const SpriteSlice& slice = tile->slice();
		QString sliceUuid = slice.id.toString(QUuid::WithoutBraces);

		// 如果这个切片还没有添加过，才添加
		if (!addedSlices.contains(sliceUuid))
		{
			addedSlices.insert(sliceUuid);
			outSliceIdMap[sliceUuid] = index;

			QJsonObject sliceObj = buildSliceData(slice);
			sliceObj["index"] = index;  // 添加索引便于引用

			slices.append(sliceObj);
			index++;
		}
	}

	return slices;
}

QJsonObject MapExporter::buildSliceData(const SpriteSlice& slice)
{
	QJsonObject sliceObj;

	// 唯一标识
	sliceObj["id"] = slice.id.toString(QUuid::WithoutBraces);
	sliceObj["name"] = slice.name;

	// 在原图中的位置
	QJsonObject sourceRect;
	sourceRect["x"] = slice.x;
	sourceRect["y"] = slice.y;
	sourceRect["width"] = slice.width;
	sourceRect["height"] = slice.height;
	sliceObj["sourceRect"] = sourceRect;

	// 锚点
	QJsonObject anchor;
	anchor["x"] = slice.anchor.x();
	anchor["y"] = slice.anchor.y();
	sliceObj["anchor"] = anchor;

	// 分组
	sliceObj["group"] = slice.group;

	// 装饰标记
	sliceObj["decorationOnly"] = slice.isDecorationOnly;

	return sliceObj;
}

QJsonArray MapExporter::buildLayers(
	const QVector<MapTileItem*>& tiles,
	int layerCount,
	const QMap<QString, int>& sliceIdMap)
{
	QJsonArray layers;

	// 按图层分组瓦片
	QVector<QVector<MapTileItem*>> layeredTiles(layerCount);
	for (auto* tile : tiles)
	{
		if (tile && tile->layer() >= 0 && tile->layer() < layerCount)
		{
			layeredTiles[tile->layer()].append(tile);
		}
	}

	// 图层名称
	static const QStringList layerNames = {
		"Background",
		"Ground",
		"Decoration",
		"Foreground"
	};

	// 构建每个图层
	for (int i = 0; i < layerCount; ++i)
	{
		QJsonObject layer;
		layer["id"] = i;
		layer["name"] = (i < layerNames.size()) ? layerNames[i] : QString("Layer %1").arg(i);
		layer["visible"] = true;
		layer["locked"] = false;
		layer["opacity"] = 1.0;

		// 图层中的瓦片
		QJsonArray tilesArray;
		for (auto* tile : layeredTiles[i])
		{
			tilesArray.append(buildTileData(tile, sliceIdMap));
		}
		layer["tiles"] = tilesArray;
		layer["tileCount"] = tilesArray.size();

		layers.append(layer);
	}

	return layers;
}

QJsonObject MapExporter::buildTileData(MapTileItem* tile, const QMap<QString, int>& sliceIdMap)
{
	QJsonObject tileObj;

	// ========== 位置信息 ==========
	QJsonObject position;
	position["gridX"] = tile->gridX();
	position["gridY"] = tile->gridY();
	position["pixelX"] = static_cast<int>(tile->pos().x());
	position["pixelY"] = static_cast<int>(tile->pos().y());
	tileObj["position"] = position;

	// ========== 尺寸信息 ==========
	QJsonObject size;
	size["gridWidth"] = tile->gridWidth();
	size["gridHeight"] = tile->gridHeight();
	size["pixelWidth"] = static_cast<int>(tile->boundingRect().width());
	size["pixelHeight"] = static_cast<int>(tile->boundingRect().height());
	tileObj["size"] = size;

	// ========== 图集引用 ==========
	tileObj["tilesetId"] = tile->tilesetId();

	// ========== 切片引用 ==========
	QString sliceUuid = tile->slice().id.toString(QUuid::WithoutBraces);
	tileObj["sliceIndex"] = sliceIdMap.value(sliceUuid, -1);
	tileObj["sliceId"] = sliceUuid;  // 保留 UUID 用于调试

	// ========== 显示名称 ==========
	tileObj["displayName"] = tile->displayName();

	// ========== 碰撞信息 ==========
	QJsonObject collision;
	CollisionType collisionType = tile->collisionType();
	collision["enabled"] = (collisionType != CollisionType::None);

	// 导出碰撞类型字符串
	QString collisionTypeStr;
	switch (collisionType)
	{
	case CollisionType::None:    collisionTypeStr = "none"; break;
	case CollisionType::Ground:  collisionTypeStr = "ground"; break;
	case CollisionType::Trigger: collisionTypeStr = "trigger"; break;
	default: collisionTypeStr = "none"; break;
	}
	collision["type"] = collisionTypeStr;
	collision["typeId"] = static_cast<int>(collisionType);
	tileObj["collision"] = collision;

	// ========== 变换信息 ==========
	QJsonObject transform;
	transform["flipX"] = tile->isFlippedX();
	transform["flipY"] = tile->isFlippedY();
	transform["rotation"] = tile->rotation();	// 角度制
	tileObj["transform"] = transform;

	// ========== 图层信息 ==========
	tileObj["layer"] = tile->layer();
	tileObj["zIndex"] = static_cast<int>(tile->zValue());

	// ========== 标签 ==========
	QJsonArray tags;
	const QString& tileTags = tile->tags();
	if (!tileTags.isEmpty())
	{
		QStringList tagList = tileTags.split(',', Qt::SkipEmptyParts);
		for (const QString& tag : tagList)
		{
			tags.append(tag.trimmed());
		}
	}
	tileObj["tags"] = tags;

	// 自定义数据（预留位置）
	QJsonObject customData;
	tileObj["customData"] = customData;

	return tileObj;
}