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

android {
	QMAKE_DISTCLEAN +=-rf android-$(TARGET)-deployment-settings.json apk *.json *.apk
	QMAKE_POST_LINK = make install INSTALL_ROOT=apk && `dirname $(QMAKE)`/androiddeployqt --input android-libscaner.so-deployment-settings.json --output apk --deployment bundled --android-platform android-21 --debug && mv apk/bin/QtApp-debug.apk scaner.apk
}

