#pragma once

#include "DocumentManager.h"

class AppContext
{
public:
	AppContext() = default;

	// 暂时只有一个成员，后面会加更多
	DocumentManager documentManager;

	// 后面可以在这里加：加载配置、加载样式等
	void loadStyle(const QString& qssPath);
};
