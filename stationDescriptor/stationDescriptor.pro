TEMPLATE = app
CONFIG -= app_bundle
CONFIG -= qt
CONFIG += console c++17
TARGET = stationDesc
QMAKE_LFLAGS+=-fopenmp
QMAKE_CXXFLAGS+=-fopenmp

INCLUDEPATH += $$PWD/../carteApt/
INCLUDEPATH += $$PWD/../cWebAptitude

SOURCES += stationDescriptor.cpp \
        ../carteApt/cdicoapt.cpp \
        ../carteApt/cnsw.cpp \
    plaiprfw.cpp \
    ../carteApt/layerbase.cpp \
    ../cWebAptitude/cadastre.cpp

LIBS = -lboost_system -lboost_iostreams -lboost_thread -lboost_filesystem -lboost_program_options -lwtdbo -lwtdbosqlite3 -fopenmp

LIBS += -L$$PWD/usr/include/ -lsqlite3
LIBS += -L$$PWD/usr/include/gdal/ -lgdal
INCLUDEPATH += $$PWD/../../../../../../usr/include/gdal/
DEPENDPATH += $$PWD/../../../../../../usr/include/gdal/

HEADERS += \
    plaiprfw.h \
    ../carteApt/layerbase.h \
    ../carteApt/cdicoapt.h \
    ../carteApt/cnsw.h \
    ../cWebAptitude/cadastre.h \

