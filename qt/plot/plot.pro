TEMPLATE = app
TARGET = plot
QT += core widgets charts

include(msg.pri)

HEADERS += main.h graph.h channel.h
SOURCES += main.cpp graph.cpp channel.cpp

FORMS += main.ui channel.ui

RESOURCES += resources.qrc

win32 {

	CONFIG += static
	CONFIG += console
	QMAKE_LFLAGS += -static -static-libgcc
}

