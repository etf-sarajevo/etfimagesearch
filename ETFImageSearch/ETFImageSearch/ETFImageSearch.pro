#-------------------------------------------------
#
# Project created by QtCreator 2012-12-04T09:32:30
#
#-------------------------------------------------

QT       += core gui

TARGET = ETFImageSearch
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
    indexer.cpp \
    searchalgorithm.cpp \
    rgbhistogram.cpp \
    liuetal_v2.cpp \
    prtest.cpp

HEADERS  += mainwindow.h \
    indexer.h \
    searchalgorithm.h \
    rgbhistogram.h \
    liuetal_v2.h \
    prtest.h

FORMS    += mainwindow.ui

INCLUDEPATH += ../libjpeg-hacked

win* {
    LIBS += ..\\libjpeg-hacked\\libjpeg.a
} else {
    LIBS += ../libjpeg-hacked/libjpeg.a
}
