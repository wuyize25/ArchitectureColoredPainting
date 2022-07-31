#include "MainWindow.h"
#include <QtWidgets/QApplication>

extern "C" {
    _declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
}

int main(int argc, char *argv[])
{
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
