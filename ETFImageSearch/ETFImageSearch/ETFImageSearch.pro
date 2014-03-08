#-------------------------------------------------
#
# Project created by QtCreator 2012-12-04T09:32:30
#
#-------------------------------------------------

QT       += core gui

# Uncomment this to enable ANN indexer
CONFIG += ann

TARGET = ETFImageSearch
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
    indexer.cpp \
    prtest.cpp \
    qcustomplot.cpp \
    colorhistogram.cpp \
    hsl10bin.cpp \
    treeindexer.cpp \
    lshindexer.cpp \
    imagefeatures.cpp \
    distancemetric.cpp \
    sequentialindexer.cpp \
    luetal_v2.cpp \
    newindexdialog.cpp \
    cedd.cpp \
    zhangetal.cpp \
    hmmdquant.cpp \
    pixel.cpp

HEADERS  += mainwindow.h \
    indexer.h \
    prtest.h \
    qcustomplot.h \
    colorhistogram.h \
    hsl10bin.h \
    treeindexer.h \
    lshindexer.h \
    imagefeatures.h \
    distancemetric.h \
    featurevector.h \
    sequentialindexer.h \
    globals.h \
    luetal_v2.h \
    newindexdialog.h \
    cedd.h \
    zhangetal.h \
    hmmdquant.h \
    pixel.h

FORMS    += mainwindow.ui \
    newindexdialog.ui

INCLUDEPATH += ../libjpeg-hacked

ann {
	SOURCES += annindexer.cpp
	HEADERS  += annindexer.h
	LIBS += -lann 
	# On some platforms use this:
	#LIBS += -lANN
	DEFINES += LIB_ANN
}

win* {
    LIBS += $$PWD/../libjpeg-hacked/libjpeg.a
} else {
    LIBS += $$PWD/../libjpeg-hacked/libjpeg.a
}
