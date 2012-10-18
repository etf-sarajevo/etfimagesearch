#-------------------------------------------------
#
# Project created by QtCreator 2011-02-02T23:18:21
#
#-------------------------------------------------

TARGET = cbir_jpeg
#TEMPLATE = lib

#DEFINES += JPEGIMAGE_LIBRARY

SOURCES += \
#    jpegimage.cpp \
#    component.cpp \
#    huffmantable.cpp \
#    quantizationtable.cpp \
#    exportpicture.cpp \
#    huffmanelementscount.cpp \
#    jpegdecode.cpp \
#    jpegencode.cpp \
    main.cpp \
    cbir.c

HEADERS += \
#    jpegimage.h\
#    JPEGImage_global.h \
#    component.h \
#    quantizationtable.h \
#    exportpicture.h \
#    huffmanelementscount.h \
#    jpegdecode.h \
#    jpegencode.h
#    huffmantable.h \
    cbir.h

#CONFIG += debug

LIBS = -L../jpeg-rad/ -ljpeg
INCLUDEPATH += ../jpeg-rad
#DESTDIR      = ../../etfshop-build-desktop/plugins





















