TEMPLATE = app
#CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt
TARGET = carteApt


QMAKE_CC = gcc-10
QMAKE_CXX = g++-10

SOURCES += main.cpp \
           cdicoapt.cpp \
    caplicarteapt.cpp \
    cdicocarteph.cpp \
    capplicarteph.cpp \
    cnsw.cpp \
    layerbase.cpp\
    ../cWebAptitude/cadastre.cpp

LIBS += -lgdal -lsqlite3 -lboost_system -lboost_filesystem -lwtdbo -lwtdbosqlite3 -lboost_program_options

#LIBS += -L$$PWD/../../../OTB-7.0.0-Linux64/lib

LIBS += -L$$PWD/usr/include/gdal/ -lgdal
INCLUDEPATH += $$PWD/../../../../../../usr/include/gdal/
DEPENDPATH += $$PWD/../../../../../../usr/include/gdal/

#LIBS += -L$$PWD/../../../OTB-7.0.0-Linux64/lib -lgdal
#INCLUDEPATH += $$PWD/../../../OTB-7.0.0-Linux64/include
#DEPENDPATH += $$PWD/../../../OTB-7.0.0-Linux64/include

# pour boost
INCLUDEPATH += $$PWD/../../../../../../../../../usr/lib/x86_64-linux-gnu
DEPENDPATH += $$PWD/../../../../../../../../../usr/lib/x86_64-linux-gnu

INCLUDEPATH += $$PWD/../cWebAptitude/
DEPENDPATH += $$PWD/../cWebAptitude/

HEADERS += \
    cdicoapt.h \
    caplicarteapt.h \
    cdicocarteph.h \
    capplicarteph.h \
    cnsw.h \
    layerbase.h
    ../cWebAptitude/cadastre.h

