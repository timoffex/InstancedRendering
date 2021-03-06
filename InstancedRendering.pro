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

# Turn off warnings about C++17 extensions because MyCLKernel will generate a LOT of them.
QMAKE_CXXFLAGS += -Wno-c++17-extensions


SOURCES += \
    src/grass.cpp \
    src/grassglprogram.cpp \
    src/grasswindclprogram.cpp \
    src/main.cpp \
    src/mainwindow.cpp \
    src/cl_interface/myclimagedescriptor.cpp \
    src/cl_interface/myclwrapper.cpp \
    src/windquadglprogram.cpp \
    src/cl_interface/myclerrors.cpp \
    src/fluid2dsimulation.cpp \
    src/cl_interface/myclimage.cpp \
    src/fluid2dsimulationclprogram.cpp \
    src/utilitiesclprogram.cpp \
    src/cl_interface/myclprogram.cpp \
    src/cl_interface/clniceties.cpp

HEADERS += \
    src/grass.h \
    src/grassglprogram.h \
    src/grasswindclprogram.h \
    src/mainwindow.h \
    src/cl_interface/myclimagedescriptor.h \
    src/cl_interface/myclwrapper.h \
    src/windquadglprogram.h \
    src/cl_interface/myclerrors.h \
    src/cl_interface/include_opencl.h \
    src/fluid2dsimulation.h \
    src/cl_interface/myclimage.h \
    src/fluid2dsimulationclprogram.h \
    src/utilitiesclprogram.h \
    src/cl_interface/myclprogram.h \
    src/cl_interface/myclkernel.h \
    src/cl_interface/clniceties.h

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
