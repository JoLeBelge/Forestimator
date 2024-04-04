TEMPLATE = app
TARGET = manip
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

INCLUDEPATH += /home/jo/app/phytospy/phytoWebTool/ranger/
INCLUDEPATH += /home/jo/app/phytospy/phytoWebTool/ranger/Tree
INCLUDEPATH += /home/jo/app/phytospy/phytoWebTool/ranger/Forest/
INCLUDEPATH += /home/jo/app/phytospy/phytoWebTool/ranger/utility
INCLUDEPATH += /home/jo/app/phytospy/phytoWebTool/phyto/
INCLUDEPATH += /home/jo/app/phytospy/phytoWebTool/tinyExif/
INCLUDEPATH += /home/jo/app/phytospy/phytoWebTool/auth/
DEPENDPATH += /home/jo/app/phytospy/phytoWebTool/phyto/
DEPENDPATH  += /home/jo/app/phytospy/phytoWebTool/ranger/
DEPENDPATH  += /home/jo/app/phytospy/phytoWebTool/ranger/Tree/
DEPENDPATH += /home/jo/app/phytospy/phytoWebTool/ranger/Forest/
DEPENDPATH += /home/jo/app/phytospy/phytoWebTool/ranger/utility/
DEPENDPATH += /home/jo/app/phytospy/phytoWebTool/tinyExif/
DEPENDPATH += /usr/include/gdal/
INCLUDEPATH += /usr/include/gdal/

SOURCES += manipRessource.cpp \
    ../carteApt/cdicoaptbase.cpp \
    analytics.cpp \
    api/cnswresource.cpp \
    auth/Session.cpp \
    auth/User.cpp \
    auth/auth.cpp \
    cadastre.cpp \
    panier.cpp \
    simplepoint.cpp \
    widgetcadastre.cpp \
    wopenlayers.cpp \
    cwebaptitude.cpp \
    ../carteApt/cdicoapt.cpp \
    layer.cpp \
    grouplayers.cpp \
    parcellaire.cpp \
    layerstatchart.cpp \
    #uploadcarte.cpp \
    libzippp/src/libzippp.cpp \
    ecogrammeEss.cpp \
    statwindow.cpp \
    selectlayers.cpp \
    presentationpage.cpp \
    ../carteApt/cnsw.cpp \
    ../carteApt/layerbase.cpp \
    api/stationdescresource.cpp \
    statHdomCompo.cpp\
    /home/jo/app/phytospy/phytoWebTool/phyto/cphyto.cpp \

HEADERS += \
    ../carteApt/cdicoaptbase.h \
    analytics.h \
    api/cnswresource.h \
    auth/Session.h \
    auth/User.h \
    auth/auth.h \
    cadastre.h \
    manipRessource.h \
    panier.h \
    simplepoint.h \
    widgetcadastre.h \
    wopenlayers.h \
    cwebaptitude.h \
    ../carteApt/cdicoapt.h \
    layer.h \
    grouplayers.h \
    parcellaire.h \
    layerstatchart.h \
    #uploadcarte.h \
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
    ../stationDescriptor/rapidxml/rapidxml_iterators.hpp \
    /home/jo/app/phytospy/phytoWebTool/phyto/cphyto.h \

HEADERS += /home/jo/app/phytospy/phytoWebTool/ranger/globals.h \
           /home/jo/app/phytospy/phytoWebTool/ranger/version.h \
           /home/jo/app/phytospy/phytoWebTool/ranger/Forest/Forest.h \
           /home/jo/app/phytospy/phytoWebTool/ranger/Forest/ForestClassification.h \
           /home/jo/app/phytospy/phytoWebTool/ranger/Forest/ForestProbability.h \
           /home/jo/app/phytospy/phytoWebTool/ranger/Forest/ForestRegression.h \
           /home/jo/app/phytospy/phytoWebTool/ranger/Forest/ForestSurvival.h \
          /home/jo/app/phytospy/phytoWebTool/ranger/Tree/Tree.h \
           /home/jo/app/phytospy/phytoWebTool/ranger/Tree/TreeClassification.h \
           /home/jo/app/phytospy/phytoWebTool/ranger/Tree/TreeProbability.h \
           /home/jo/app/phytospy/phytoWebTool/ranger/Tree/TreeRegression.h \
           /home/jo/app/phytospy/phytoWebTool/ranger/Tree/TreeSurvival.h \
           /home/jo/app/phytospy/phytoWebTool/ranger/utility/ArgumentHandler.h \
           /home/jo/app/phytospy/phytoWebTool/ranger/utility/Data.h \
           /home/jo/app/phytospy/phytoWebTool/ranger/utility/DataChar.h \
           /home/jo/app/phytospy/phytoWebTool/ranger/utility/DataDouble.h \
           /home/jo/app/phytospy/phytoWebTool/ranger/utility/DataFloat.h \
           /home/jo/app/phytospy/phytoWebTool/ranger/utility/utility.h

SOURCES +=  /home/jo/app/phytospy/phytoWebTool/ranger/Forest/Forest.cpp\
            /home/jo/app/phytospy/phytoWebTool/ranger/Forest/ForestClassification.cpp \
           /home/jo/app/phytospy/phytoWebTool/ranger/Forest/ForestProbability.cpp \
           /home/jo/app/phytospy/phytoWebTool/ranger/Forest/ForestRegression.cpp \
           /home/jo/app/phytospy/phytoWebTool/ranger/Forest/ForestSurvival.cpp \
           /home/jo/app/phytospy/phytoWebTool/ranger/Tree/Tree.cpp \
           /home/jo/app/phytospy/phytoWebTool/ranger/Tree/TreeClassification.cpp \
           /home/jo/app/phytospy/phytoWebTool/ranger/Tree/TreeProbability.cpp \
           /home/jo/app/phytospy/phytoWebTool/ranger/Tree/TreeRegression.cpp \
           /home/jo/app/phytospy/phytoWebTool/ranger/Tree/TreeSurvival.cpp \
           /home/jo/app/phytospy/phytoWebTool/ranger/utility/ArgumentHandler.cpp \
           /home/jo/app/phytospy/phytoWebTool/ranger/utility/Data.cpp \
           /home/jo/app/phytospy/phytoWebTool/ranger/utility/DataChar.cpp \
           /home/jo/app/phytospy/phytoWebTool/ranger/utility/DataFloat.cpp \
           /home/jo/app/phytospy/phytoWebTool/ranger/utility/utility.cpp\


