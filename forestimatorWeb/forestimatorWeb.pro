TEMPLATE = app
TARGET = forestimatorWeb
INCLUDEPATH += .
CONFIG -= qt
QT -= gui
QT += sql
CONFIG += c++17

LIBS = -lcurl -lgdal -lwthttp -lwt -lboost_system -lboost_iostreams  -lboost_filesystem -lboost_program_options -lcrypt -pthread -lwtdbo -lwtdbosqlite3 -lzip -lhpdf -lsqlite3

DEPENDPATH += /usr/include/gdal/
INCLUDEPATH += /usr/include/gdal/

INCLUDEPATH += $$PWD/../carteApt/
INCLUDEPATH += $$PWD/auth/
INCLUDEPATH += $$PWD/libzipp/src/
DEPENDPATH += $$PWD/libzipp/src/


INCLUDEPATH += $$PWD../stationDescriptor/rapidxml/
DEPENDPATH += $$PWD../stationDescriptor/rapidxml/

SOURCES += main.cpp \
    ../carteApt/cdicoaptbase.cpp \
    analytics.cpp \
    api/anaponctuelleresource.cpp \
    api/cnswresource.cpp \
    api/rasterclipresource.cpp \
    auth/Session.cpp \
    auth/User.cpp \
    cadastre.cpp \
    ../carteApt/matapt.cpp \
    formviellecouperase.cpp \
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
    statHdomCompo.cpp

HEADERS += \
    ../carteApt/cdicoaptbase.h \
    analytics.h \
    api/anaponctuelleresource.h \
    api/cnswresource.h \
    api/rasterclipresource.h \
    auth/Session.h \
    auth/User.h \
    cadastre.h \
    formviellecouperase.h \
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
    statHdomCompo.h \
    ../stationDescriptor/rapidxml/rapidxml.hpp \
    ../stationDescriptor/rapidxml/rapidxml_iterators.hpp
