TEMPLATE = lib

CONFIG += staticlib
TARGET = qtethminer

include(qtethminer.pri)

SOURCES += ethereumminer.cpp \
    stratumclient.cpp
HEADERS += ethereumminer.h \
    stratumclient.h
