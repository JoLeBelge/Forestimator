TEMPLATE = app
TARGET = WebAptitude
INCLUDEPATH += .
CONFIG -= qt
QT -= gui
QT += sql

CONFIG += c++11

# la compilation sous debian avec gcc-8 m'a montré un bug dans la boucle de création des groupes écologiques. je spécifie donc que c'est avec le compilateur g++-7 qu'il faut compiler le soft
# update ; maintenant le code est compatible avec gcc 5 et 9
QMAKE_CC = gcc-8
QMAKE_CXX = g++-8


#
LIBS = -lgdal -lwthttp -lwt -lboost_system -lboost_iostreams -lboost_thread -lboost_filesystem -lboost_program_options -lcrypt -pthread -lwtdbo -lwtdbosqlite3

INCLUDEPATH += $$PWD/../carteApt/



contains(pl,serveur) {
LIBS += -L$$PWD/usr/include/gdal/ -lgdal
INCLUDEPATH += $$PWD/../../../usr/include/gdal/
DEPENDPATH += $$PWD/../../../usr/include/gdal/
# sur le serveur je dois lui préciser ou est cette librairie
LIBS += -L$$PWD/../../../usr/lib/x86_64-linux-gnu/ -lsqlite3
} else {
LIBS += -L$$PWD/../../../usr/include/ -lsqlite3
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
    legend.cpp \
    parcellaire.cpp

HEADERS += \
    wopenlayers.h \
    cwebaptitude.h \
    ../carteApt/cdicoapt.h \
    layer.h \
    grouplayers.h \
    legend.h \
    parcellaire.h
