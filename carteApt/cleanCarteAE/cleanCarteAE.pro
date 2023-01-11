TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt
TARGET = cleanAE

QT += x11extras

SOURCES += \
        main.cpp\

LIBS += -L/home/jo/app/micmac/build/src/ -lelise
PRE_TARGETDEPS += /home/jo/app/micmac/build/src/libelise.a

LIBS += -lX11 -lboost_program_options -lboost_filesystem

LIBS += -L$$PWD/usr/include/gdal/ -lgdal
INCLUDEPATH += /usr/include/gdal/
DEPENDPATH += /usr/include/gdal/

INCLUDEPATH += $$PWD/../../../micmac/include/
DEPENDPATH += $$PWD/../../../micmac/include/

HEADERS += \
    cleanAE.h
