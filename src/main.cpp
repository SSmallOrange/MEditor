#include <QApplication>
#include "app/AppContext.h"
#include "ui/MainWindow.h"

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);

	AppContext ctx;

	// 如果你暂时没有 qss 文件，这行可以先注释掉
	ctx.loadStyle(":/mac_light.qss");
	 
	MainWindow w(&ctx);
	w.show();

	return app.exec();
}
