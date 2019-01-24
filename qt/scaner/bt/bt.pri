DEPENDPATH += $$PWD/
INCLUDEPATH += $$PWD/
INCLUDEPATH += $$PWD/

QT += core widgets bluetooth

QMAKE_CXXFLAGS += -g -std=gnu++0x -O0
QMAKE_CFLAGS += -g -O0

HEADERS += $$PWD/wdg_bt.h $$PWD/bt_list.h
SOURCES += $$PWD/wdg_bt.cpp $$PWD/bt_list.cpp
FORMS += $$PWD/bt.ui

RESOURCES += $$PWD/bt_resources.qrc

include(../msg.pri)

