TEMPLATE = app
TARGET = WebAptitude
INCLUDEPATH += .
CONFIG -= qt
QT -= gui
QT += sql

CONFIG += c++11

# la compilation sous debian avec gcc-8 m'a montré un bug dans la boucle de création des groupes écologiques. je spécifie donc que c'est avec le compilateur g++-7 qu'il faut compiler le soft
# update ; maintenant le code est compatible avec gcc 5 et 9
QMAKE_CC = gcc-9
QMAKE_CXX = g++-9


#
LIBS = -lgdal -lwthttp -lwt -lboost_system -lboost_iostreams -lboost_thread -lboost_filesystem -lboost_program_options -lcrypt -pthread -lwtdbo -lwtdbosqlite3 -lzip #-lzippp_static

INCLUDEPATH += $$PWD/../carteApt/

# libzippp version https://github.com/ctabin/libzippp
#LIBS += -L$$PWD/../../../../../../usr/local/lib -lzippp_static#INCLUDEPATH += $$PWD/../../../../../../usr/local/include/libzippp/
#DEPENDPATH += $$PWD/../../../../../../usr/local/include/libzippp/

#qmake -makefile ../cWebAptitude/cWebAptitude.pro pl=server avant de lancer make sur debian server
contains(pl,serveur) {
LIBS += -L$$PWD/usr/include/gdal/ -lgdal
INCLUDEPATH += $$PWD/../../../usr/include/gdal/
DEPENDPATH += $$PWD/../../../usr/include/gdal/

# sur le serveur je dois lui préciser ou est cette librairie
LIBS += -L$$PWD/../../../usr/lib/x86_64-linux-gnu/ -lsqlite3

INCLUDEPATH += $$PWD/libzipp/src/
DEPENDPATH += $$PWD/libzipp/src/

} else {
LIBS += -L$$PWD/../../../usr/include/ -lsqlite3
LIBS += -L$$PWD/usr/include/gdal/ -lgdal
INCLUDEPATH += $$PWD/../../../../../../usr/include/gdal/
DEPENDPATH += $$PWD/../../../../../../usr/include/gdal/
INCLUDEPATH += $$PWD/libzipp/src/
DEPENDPATH += $$PWD/libzipp/src/

}

SOURCES += main.cpp \
    Session.cpp \
    User.cpp \
    auth.cpp \
    wopenlayers.cpp \
    cwebaptitude.cpp \
    ../carteApt/cdicoapt.cpp \
    layer.cpp \
    grouplayers.cpp \
    legend.cpp \
    parcellaire.cpp \
    layerstatchart.cpp \
    uploadcarte.cpp \
    libzippp/src/libzippp.cpp \
    ecogrammeEss.cpp \
    stackinfoptr.cpp \
    statwindow.cpp

HEADERS += \
    Session.h \
    User.h \
    auth.h \
    main.h \
    wopenlayers.h \
    cwebaptitude.h \
    ../carteApt/cdicoapt.h \
    layer.h \
    grouplayers.h \
    legend.h \
    parcellaire.h \
    layerstatchart.h \
    uploadcarte.h \
    libzippp/src/libzippp.h \
    ecogrammeEss.h \
    stackinfoptr.h \
    statwindow.h
