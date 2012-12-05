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
    rgbhistogram.cpp

HEADERS  += mainwindow.h \
    indexer.h \
    searchalgorithm.h \
    rgbhistogram.h

FORMS    += mainwindow.ui

win* {
    LIBS += ..\\libjpeg-hacked\\libjpeg.a
} else {
    LIBS += ../libjpeg-hacked/libjpeg.a
}
