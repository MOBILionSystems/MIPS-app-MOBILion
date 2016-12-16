#-------------------------------------------------
#
# Project created by QtCreator 2015-06-27T10:54:52
#
#-------------------------------------------------

QT       += core
QT       += gui
QT       += serialport
QT       += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MIPS
TEMPLATE = app

CONFIG += static

SOURCES += main.cpp\
        mips.cpp \
        console.cpp \
        settingsdialog.cpp \
        ringbuffer.cpp \
        pse.cpp \
        comms.cpp \
        twave.cpp \
    dcbias.cpp \
    dio.cpp \
    rfdriver.cpp \
    psg.cpp \
    program.cpp \
    help.cpp \
    arb.cpp

HEADERS  += mips.h \
    console.h \
    settingsdialog.h \
    ringbuffer.h \
    pse.h \
    comms.h \
    twave.h \
    dcbias.h \
    dio.h \
    rfdriver.h \
    psg.h \
    program.h \
    help.h \
    arb.h

FORMS    += mips.ui \
    settingsdialog.ui \
    pse.ui \
    help.ui

RESOURCES += \
    files.qrc

QMAKE_LFLAGS_WINDOWS = /SUBSYSTEM:WINDOWS,5.01
QMAKE_MAC_SDK = macosx10.12


