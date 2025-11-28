TEMPLATE = app
TARGET = forestimatorWeb
INCLUDEPATH += .
CONFIG -= qt
QT -= gui
QT += sql
CONFIG += c++17

# Enable warnings
QMAKE_CXXFLAGS += -Wall -Wextra -Wpedantic

LIBS = -lcurl -lwthttp -lwt -lboost_system -lboost_iostreams  -lboost_filesystem -lboost_program_options -lcrypt -pthread -lwtdbo -lwtdbosqlite3 -lzip -lhpdf -lsqlite3

DEPENDPATH += /usr/include/gdal/
INCLUDEPATH += /usr/include/gdal/

LIBS += -L$$PWD/usr/include/gdal/ -lgdal

INCLUDEPATH += $$PWD/../carteApt/
INCLUDEPATH += $$PWD/auth/
INCLUDEPATH += $$PWD/libzipp/src/
DEPENDPATH += $$PWD/libzipp/src/


INCLUDEPATH += $$PWD../stationDescriptor/rapidxml/
DEPENDPATH += $$PWD../stationDescriptor/rapidxml/

SOURCES += main.cpp \
    ../carteApt/cdicoaptbase.cpp \
    ACR/terrainviellecouperase.cpp \
    #ACR/formviellecouperase.cpp \
    OGF/formOGF.cpp \
    api/anasurfresource.cpp \
    desserteForest/desserteForest.cpp \
    analytics.cpp \
    api/anaponctuelleresource.cpp \
    api/rasterclipresource.cpp \
    api/staticmapresource.cpp \
    auth/Session.cpp \
    auth/User.cpp \
    cadastre.cpp \
    ../carteApt/matapt.cpp \
    mataptcs.cpp \
    panier.cpp \
    simplepoint.cpp \
    threadpool/Pool.cpp \
    threadpool/Task.cpp \
    threadpool/ThreadPool.cpp \
    threadpool/ThreadQueue.cpp \
    widgetcadastre.cpp \
    wopenlayers.cpp \
    cwebaptitude.cpp \
    ../carteApt/cdicoapt.cpp \
    layer.cpp \
    grouplayers.cpp \
    parcellaire.cpp \
    layerstatchart.cpp \
    libzippp/src/libzippp.cpp \
    ecogrammeEss.cpp \
    statwindow.cpp \
    selectlayers.cpp \
    presentationpage.cpp \
    ../carteApt/cnsw.cpp \
    ../carteApt/layerbase.cpp \
    api/stationdescresource.cpp \
    tools/tools.cpp \

HEADERS += \
    ../carteApt/cdicoaptbase.h \
    ACR/terrainviellecouperase.h \
    #ACR/formviellecouperase.h \
    OGF/formOGF.h \
    api/anasurfresource.h \
    desserteForest/desserteForest.h \
    analytics.h \
    api/anaponctuelleresource.h \
    api/rasterclipresource.h \
    api/staticmapresource.h \
    auth/Session.h \
    auth/User.h \
    cadastre.h \
    main.h \
    ../carteApt/matapt.h \
    mataptcs.h \
    panier.h \
    simplepoint.h \
    threadpool/Pool.hpp \
    threadpool/Task.hpp \
    threadpool/ThreadPool.hpp \
    threadpool/ThreadQueue.hpp \
    widgetcadastre.h \
    wopenlayers.h \
    cwebaptitude.h \
    ../carteApt/cdicoapt.h \
    layer.h \
    grouplayers.h \
    parcellaire.h \
    layerstatchart.h \
    libzippp/src/libzippp.h \
    ecogrammeEss.h \
    statwindow.h \
    selectlayers.h \
    presentationpage.h \
    ../carteApt/cnsw.h \
    ../carteApt/layerbase.h \
    api/stationdescresource.h \
    ../carteApt/color.h \
    ../stationDescriptor/rapidxml/rapidxml.hpp \
    ../stationDescriptor/rapidxml/rapidxml_iterators.hpp \
    tools/tools.hpp \

