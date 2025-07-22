TEMPLATE = app
CONFIG -= app_bundle
CONFIG -= qt
CONFIG += console c++17
TARGET = stationDesc
QMAKE_LFLAGS+=-fopenmp
QMAKE_CXXFLAGS+=-fopenmp

#QMAKE_CC = gcc-9
#QMAKE_CXX = g++-9
#sudo ln -s /usr/bin/gcc-9 /usr/bin/g++-9


INCLUDEPATH += $$PWD/../carteApt/
INCLUDEPATH += $$PWD/../cWebAptitude

SOURCES += stationDescriptor.cpp \
        ../carteApt/cdicoapt.cpp \
        ../carteApt/cdicoaptbase.cpp \
        ../carteApt/cnsw.cpp \
    plaiprfw.cpp \
    ../carteApt/layerbase.cpp \
    ../cWebAptitude/cadastre.cpp \

#../../html2text/HTMLDriver.cpp \
 #../../html2text/HTMLControl.cpp \
#../../html2text/HTMLParser.tab.cc \
#../../html2text/HTMLParser.yy \



LIBS = -lboost_system -lboost_iostreams -lboost_thread -lboost_filesystem -lboost_program_options -lwtdbo -lwtdbosqlite3 -fopenmp -lwt

LIBS += -L$$PWD/usr/include/ -lsqlite3
LIBS += -L$$PWD/usr/include/gdal/ -lgdal
INCLUDEPATH += $$PWD/../../../../../../usr/include/gdal/
DEPENDPATH += $$PWD/../../../../../../usr/include/gdal/

#INCLUDEPATH += $$PWD/../../html2text/
#DEPENDPATH += $$PWD/../../html2text/

#INCLUDEPATH += $$PWD/rapidxml/
#DEPENDPATH += $$PWD/rapidxml/

HEADERS += \
    plaiprfw.h \
    ../carteApt/layerbase.h \
    ../carteApt/cdicoapt.h \
    ../carteApt/cdicoaptbase.h \
    ../carteApt/cnsw.h \
    ../cWebAptitude/cadastre.h \
 #   rapidxml/rapidxml.hpp \
 #   rapidxml/rapidxml_iterators.hpp \
  #  ../../html2text/HTMLDriver.h \
  #  ../../html2text/HTMLControl.h \
#../../html2text/HTMLParser.tab.hh \

