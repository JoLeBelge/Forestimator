TEMPLATE = app
TARGET = WebAptitude
INCLUDEPATH += .
CONFIG -= qt
QT -= gui
QT += sql

CONFIG += c++11

# la compilation sous debian avec gcc-8 m'a montré un bug dans la boucle de création des groupes écologiques. je spécifie donc que c'est avec le compilateur g++-7 qu'il faut compiler le soft
# update ; maintenant le code est compatible avec gcc 5 et 9. mais sur le serveur c'est gcc 8 pas 9, le 9 n'est pas installé. En plus 8 fonctionne très bien
QMAKE_CC = gcc-8
QMAKE_CXX = g++-8


#
LIBS = -lgdal -lwthttp -lwt -lboost_system -lboost_iostreams -lboost_thread -lboost_filesystem -lboost_program_options -lcrypt -pthread -lwtdbo -lwtdbosqlite3 -lzip -lhpdf

INCLUDEPATH += $$PWD/../carteApt/
INCLUDEPATH += $$PWD/auth/

# libzippp version https://github.com/ctabin/libzippp
#LIBS += -L$$PWD/../../../../../../usr/local/lib -lzippp_static#INCLUDEPATH += $$PWD/../../../../../../usr/local/include/libzippp/
#DEPENDPATH += $$PWD/../../../../../../usr/local/include/libzippp/

#qmake -makefile ../cWebAptitude/cWebAptitude.pro pl=serveur avant de lancer make sur debian server
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
    auth/Session.cpp \
    auth/User.cpp \
    auth/auth.cpp \
    wopenlayers.cpp \
    cwebaptitude.cpp \
    ../carteApt/cdicoapt.cpp \
    layer.cpp \
    grouplayers.cpp \
    simplepoint.cpp \
    parcellaire.cpp \
    layerstatchart.cpp \
    uploadcarte.cpp \
    libzippp/src/libzippp.cpp \
    ecogrammeEss.cpp \
    stackinfoptr.cpp \
    statwindow.cpp \
    selectlayers.cpp \
    presentationpage.cpp

HEADERS += \
    auth/Session.h \
    auth/User.h \
    auth/auth.h \
    main.h \
    wopenlayers.h \
    cwebaptitude.h \
    ../carteApt/cdicoapt.h \
    layer.h \
    grouplayers.h \
    simplepoint.h \
    parcellaire.h \
    layerstatchart.h \
    uploadcarte.h \
    libzippp/src/libzippp.h \
    ecogrammeEss.h \
    stackinfoptr.h \
    statwindow.h \
    selectlayers.h \
    presentationpage.h
