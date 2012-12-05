TEMPLATE = lib
TARGET = jpeg

CONFIG += console
CONFIG -= qt
CONFIG += staticlib

SOURCES += \
    jutils.c \
    jquant2.c \
    jquant1.c \
    jmemnobs.c \
    jmemmgr.c \
    jidctred.c \
    jidctint.c \
    jidctfst.c \
    jidctflt.c \
    jfdctint.c \
    jfdctfst.c \
    jfdctflt.c \
    jerror.c \
    jdtrans.c \
    jdsample.c \
    jdpostct.c \
    jdphuff.c \
    jdmerge.c \
    jdmaster.c \
    jdmarker.c \
    jdmainct.c \
    jdinput.c \
    jdhuff.c \
    jddctmgr.c \
    jdcolor.c \
    jdcoefct.c \
    jdatasrc.c \
    jdatadst.c \
    jdapistd.c \
    jdapimin.c \
    jctrans.c \
    jcsample.c \
    jcprepct.c \
    jcphuff.c \
    jcparam.c \
    jcomapi.c \
    jcmaster.c \
    jcmarker.c \
    jcmainct.c \
    jcinit.c \
    jchuff.c \
    jcdctmgr.c \
    jccolor.c \
    jccoefct.c \
    jcapistd.c \
    jcapimin.c \
    ckconfig.c

HEADERS += \
    transupp.h \
    jversion.h \
    jpeglib.h \
    jpegint.h \
    jmorecfg.h \
    jmemsys.h \
    jinclude.h \
    jerror.h \
    jdhuff.h \
    jdct.h \
    jchuff.h \
    cdjpeg.h \
    cderror.h \
    jconfig.h


