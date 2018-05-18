#-------------------------------------------------
#
# Project created by QtCreator 2018-05-10T17:34:48
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# This includes the OpenCL library on a Mac.
# If compiling for a different platform, you will likely get a
# symbols not found error. The solution is to add the appropriate
# library options here.
macx: {
    LIBS += -framework OpenCL
}

TARGET = InstancedRendering
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
    src/grass.cpp \
    src/grassglprogram.cpp \
    src/grasswindclprogram.cpp \
    src/imagetestwindow.cpp \
    src/main.cpp \
    src/mainwindow.cpp \
    src/myclimage_rgba32f.cpp \
    src/myclimagedescriptor.cpp \
    src/myclwrapper.cpp \
    src/windquadglprogram.cpp

HEADERS += \
    src/grass.h \
    src/grassglprogram.h \
    src/grasswindclprogram.h \
    src/imagetestwindow.h \
    src/mainwindow.h \
    src/myclimage_rgba32f.h \
    src/myclimagedescriptor.h \
    src/myclwrapper.h \
    src/windquadglprogram.h

DISTFILES += \
    src/grass_blade.jpg \
    src/grassWindReact.cl \
    src/shaders/grass.frag \
    src/shaders/windQuad.frag \
    src/shaders/grass.vert \
    src/shaders/texturedNDC.vert

RESOURCES += \
    src/assets.qrc \
    src/shaders.qrc
