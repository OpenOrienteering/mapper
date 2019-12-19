#-------------------------------------------------
#
# Project created by QtCreator 2016-12-29T20:11:49
#
#-------------------------------------------------

QT       -= core gui

TARGET = potrace
TEMPLATE = lib
CONFIG += staticlib

DEFINES += POTRACE_LIBRARY

SOURCES += \
    curve.c \
    potracelib.c \
    trace.c

HEADERS += \
    auxiliary.h \
    config.h \
    curve.h \
    lists.h \
    potracelib.h \
    progress.h \
    trace.h
