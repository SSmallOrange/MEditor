#include <QApplication>
#include <QMainWindow>

#include <iostream>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QMainWindow window;
    window.setWindowTitle("MapEditor");
    window.resize(1280, 720);
    window.show();

	std::cout << "Application started with " << argc << " arguments." << std::endl;

    return app.exec();
}
