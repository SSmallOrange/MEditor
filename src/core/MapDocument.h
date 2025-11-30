#pragma once

#include <QString>

struct MapDocument
{
	// 地图格子尺寸（单位：格数，不是像素）
	int width = 0;
	int height = 0;

	// 每个格子的像素尺寸
	int tileWidth = 0;
	int tileHeight = 0;

	QString name;

	MapDocument() = default;

	MapDocument(int w, int h, int tileW, int tileH, const QString& n = QString())
		: width(w)
		, height(h)
		, tileWidth(tileW)
		, tileHeight(tileH)
		, name(n)
	{
	}
};
