TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt
TARGET = carteApt

SOURCES += main.cpp \
           cdicoapt.cpp \
    caplicarteapt.cpp \
    cdicocarteph.cpp \
    capplicarteph.cpp

LIBS += -lgdal -lsqlite3 -lboost_system -lboost_filesystem

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

HEADERS += \
    cdicoapt.h \
    caplicarteapt.h \
    cdicocarteph.h \
    capplicarteph.h

