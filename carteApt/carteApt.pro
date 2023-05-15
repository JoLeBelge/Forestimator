TEMPLATE = app
CONFIG -= app_bundle
CONFIG -= qt
TARGET = carteApt
QMAKE_LFLAGS+=-fopenmp
QMAKE_CXXFLAGS+=-fopenmp

SOURCES += main.cpp \
    cdicoaptbase.cpp \
    cdicoapt.cpp \
    caplicarteapt.cpp \
    cdicocarteph.cpp \
    capplicarteph.cpp \
    cnsw.cpp \
    layerbase.cpp\
    ../cWebAptitude/cadastre.cpp

LIBS += -lgdal -lsqlite3 -lboost_system -lboost_filesystem -lwt -lwtdbo -lwtdbosqlite3 -lboost_program_options -fopenmp

DEPENDPATH += /usr/include/gdal/
INCLUDEPATH += /usr/include/gdal/

INCLUDEPATH += $$PWD/../cWebAptitude/
DEPENDPATH += $$PWD/../cWebAptitude/

INCLUDEPATH += $$PWD/date/include/date/
DEPENDPATH += $$PWD/date/include/date/

HEADERS += \
    date.h \
    cdicoaptbase.h \
    cdicoapt.h \
    caplicarteapt.h \
    cdicocarteph.h \
    capplicarteph.h \
    cnsw.h \
    layerbase.h
    ../cWebAptitude/cadastre.h
