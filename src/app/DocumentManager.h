#pragma once

#include <QObject>
#include "../core/MapDocument.h"

class DocumentManager : public QObject
{
	Q_OBJECT
public:
	explicit DocumentManager(QObject* parent = nullptr);

	MapDocument* document();         // 当前文档指针（可能为 nullptr）
	const MapDocument* document() const;

	bool hasDocument() const { return m_hasDocument; }

	// 新建一个简单的默认文档
	void newDefaultDocument();

signals:
	// 文档整体变化的信号（新建/加载）
	void documentChanged();

private:
	MapDocument m_document;
	bool        m_hasDocument = false;
};
