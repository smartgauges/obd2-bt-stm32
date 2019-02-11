TEMPLATE = app
TARGET = scaner
QT += core widgets

include(msg.pri)
include(bt/bt.pri)
include(com/com.pri)

HEADERS += main.h list.h wdg_fw.h wdg_rpm.h
SOURCES += main.cpp list.cpp wdg_fw.cpp wdg_rpm.cpp
SOURCES += ../../crc_xmodem.c

FORMS += main.ui fw.ui

RESOURCES += resources.qrc

win32 {

	CONFIG += static
	QMAKE_LFLAGS += -static -static-libgcc
}

