TEMPLATE = app
TARGET = WebAptitude
INCLUDEPATH += .
CONFIG -= qt
QT -= gui
QT += sql
#QMAKE_CXXFLAGS += -ggdb3 # pour utiliser valgrind et pointer la ligne du code qui va pas
#QMAKE_CXXFLAGS += -g

CONFIG += c++17

LIBS = -lcurl -lgdal -lwthttp -lwt -lboost_system -lboost_iostreams  -lboost_filesystem -lboost_program_options -lcrypt -pthread -lwtdbo -lwtdbosqlite3 -lzip -lhpdf -lsqlite3
#LIBS += -lwkhtmltox -lboost_serialization

# utilisation de GM ; fonctionne bien, mais un peu chiant à parametrer pour la compilation sur le serveur. Donc je retire et j'utilise WRasterImage (wrapper pour GM en fait)
#https://stackoverflow.com/questions/33659208/using-magick-in-qt-creator
#QMAKE_CXXFLAGS += $(shell Magick++-config --cppflags --cxxflags)
#QMAKE_CXXFLAGS += $(shell MagickCore-config --cppflags --cxxflags)
#QMAKE_CXXFLAGS += -I/usr/local/include/GraphicsMagick -pthread
#LIBS += $(shell Magick++-config --ldflags --libs)
#LIBS += $(shell MagickCore-config --ldflags --libs)
# j'ai fait tourner le shell dans la console et il m'a retourné la ligne ci-dessous, que j'utilise directement plutôt que la ligne ci-dessus
#LIBS += -L/usr/local/lib -lGraphicsMagick++ -lGraphicsMagick -ljbig -lwebp -lwebpmux -llcms2 -ltiff -lfreetype -ljpeg -lpng16 -lwmflite -lXext -lSM -lICE -lX11 -llzma -lbz2 -lxml2 -lz -lzstd -lm -lpthread -lgomp
#INCLUDEPATH +=/usr/local/include/GraphicsMagick/


INCLUDEPATH += $$PWD/../carteApt/
INCLUDEPATH += $$PWD/auth/
# libzippp version https://github.com/ctabin/libzippp
INCLUDEPATH += $$PWD/libzipp/src/
DEPENDPATH += $$PWD/libzipp/src/

#qmake -makefile ../cWebAptitude/cWebAptitude.pro pl=serveur avant de lancer make sur debian server

#QMAKE_CC = gcc-10
#QMAKE_CXX = g++-10

contains(pl,serveur) {
LIBS += -L$$PWD/usr/include/gdal/ -lgdal
INCLUDEPATH += $$PWD/../../../usr/include/gdal/
DEPENDPATH += $$PWD/../../../usr/include/gdal/

# sur le serveur je dois lui préciser ou est cette librairie
#LIBS += -L$$PWD/../../../usr/lib/x86_64-linux-gnu/ -lsqlite3

} else {
# la compilation sous debian avec gcc-8 m'a montré un bug dans la boucle de création des groupes écologiques. je spécifie donc que c'est avec le compilateur g++-7 qu'il faut compiler le soft
# update ; maintenant le code est compatible avec gcc 5 et 9.

LIBS += -L$$PWD/usr/include/gdal/ -lgdal
INCLUDEPATH += $$PWD/../../../../../../usr/include/gdal/
DEPENDPATH += $$PWD/../../../../../../usr/include/gdal/

# test de setRefence, gdal 3
#LIBS += -L$$PWD/usr/local/include/gdal/ -lgdal
#INCLUDEPATH += $$PWD/../../../../../../usr/localinclude/gdal/
#DEPENDPATH += $$PWD/../../../../../../usr/localinclude/gdal/

#INCLUDEPATH += $$PWD/../../../../../../../../../usr/lib/x86_64-linux-gnu
#DEPENDPATH += $$PWD/../../../../../../../../../usr/lib/x86_64-linux-gnu
}

SOURCES += main.cpp \
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
    uploadcarte.cpp \
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
    api/cnswresource.h \
    auth/Session.h \
    auth/User.h \
    auth/auth.h \
    cadastre.h \
    main.h \
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
    uploadcarte.h \
    libzippp/src/libzippp.h \
    ecogrammeEss.h \
    statwindow.h \
    selectlayers.h \
    presentationpage.h \
    ../carteApt/cnsw.h \
    ../carteApt/layerbase.h \
    api/stationdescresource.h \
    ../carteApt/color.h \
    statHdomCompo.h
