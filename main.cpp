#include "mainwindow.h"


#include <QApplication>
#include <QSurfaceFormat>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QSurfaceFormat format;
    format.setSamples(16);
    format.setMajorVersion(3);
    format.setMinorVersion(3);
    format.setProfile(QSurfaceFormat::CoreProfile);

    MainWindow w;
    w.setFormat(format);
    w.resize(640, 480);
    w.show();

    return a.exec();
}
