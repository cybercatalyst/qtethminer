TEMPLATE = lib

CONFIG += staticlib
TARGET = qtethminer

include(qtethminer.pri)

SOURCES += ethereumminer.cpp \
    stratumclient.cpp \
    ethereumprotocol.cpp
HEADERS += ethereumminer.h \
    stratumclient.h \
    ethereumprotocol.h
