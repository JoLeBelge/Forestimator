TEMPLATE = app
TARGET = WebAptitude
INCLUDEPATH += .
CONFIG -= qt
QT -= gui
QT += sql

# la compilation sous debian avec gcc-8 m'a montré un bug dans la boucle de création des groupes écologiques. je spécifie donc que c'est avec le compilateur g++-7 qu'il faut compiler le soft
QMAKE_CC = gcc-7
QMAKE_CXX = g++-7

LIBS = -lgdal -lwthttp -lwt -lboost_system -lboost_iostreams -lboost_thread -lboost_filesystem -lboost_program_options -lwtdbo -lwtdbosqlite3 -lcrypt -pthread

INCLUDEPATH += $$PWD/../carteApt/


contains(pl,serveur) {
LIBS += -L$$PWD/usr/include/gdal/ -lgdal
INCLUDEPATH += $$PWD/../../../usr/include/gdal/
DEPENDPATH += $$PWD/../../../usr/include/gdal/
} else {
LIBS += -L$$PWD/usr/include/gdal/ -lgdal
INCLUDEPATH += $$PWD/../../../../../../usr/include/gdal/
DEPENDPATH += $$PWD/../../../../../../usr/include/gdal/
}

SOURCES += main.cpp \
    wopenlayers.cpp \
    cwebaptitude.cpp \
    ../carteApt/cdicoapt.cpp \
    layer.cpp \
    grouplayers.cpp \
    legend.cpp

HEADERS += \
    wopenlayers.h \
    cwebaptitude.h \
    ../carteApt/cdicoapt.h \
    layer.h \
    grouplayers.h \
    legend.h
