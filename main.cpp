#include "mainwindow.h"
#include "imagetestwindow.h"

#include <QApplication>
#include <QSurfaceFormat>


#include <cstdlib>
#include <ctime>

int main(int argc, char *argv[])
{
    srand(time(0));

    QApplication a(argc, argv);

    QSurfaceFormat format;
    format.setSamples(16);
    format.setMajorVersion(3);
    format.setMinorVersion(3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setSwapInterval(1);

    MainWindow w;
//    ImageTestWindow w;
    w.setFormat(format);
    w.resize(640, 480);
    w.show();

    return a.exec();
}
