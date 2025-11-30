#include "DocumentManager.h"

DocumentManager::DocumentManager(QObject* parent)
	: QObject(parent)
{
}

MapDocument* DocumentManager::document()
{
	return m_hasDocument ? &m_document : nullptr;
}

const MapDocument* DocumentManager::document() const
{
	return m_hasDocument ? &m_document : nullptr;
}

void DocumentManager::newDefaultDocument()
{
	// 简单先搞一个 50x30, 每格 32x32 像素的地图
	m_document = MapDocument(50, 30, 32, 32, QStringLiteral("Untitled Map"));
	m_hasDocument = true;
	emit documentChanged();
}
