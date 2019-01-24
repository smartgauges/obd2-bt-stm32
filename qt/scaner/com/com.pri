DEPENDPATH += $$PWD/
INCLUDEPATH += $$PWD/
INCLUDEPATH += $$PWD/

QMAKE_CXXFLAGS += -g -std=gnu++0x -O0
QMAKE_CFLAGS += -g -O0

HEADERS += $$PWD/wdg_com.h
SOURCES += $$PWD/wdg_com.cpp
FORMS += $$PWD/com.ui

include(../msg.pri)

QT += core widgets serialport

