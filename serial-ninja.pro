#-------------------------------------------------
#
# Project created by QtCreator 2015-06-10T16:52:08
#
#-------------------------------------------------

# Rquires Qt 5.2.1

QT       += core gui serialport uitools

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

GIT_VERSION = $$system(git --git-dir $$PWD/.git --work-tree $$PWD describe --always --tags)
DEFINES += GIT_VERSION=\\"$$GIT_VERSION\\"

GIT_VERSION = "v0.23"

REVISION = $$system(git rev-parse HEAD)
DEFINES += APP_REVISION=\\\"$$REVISION\\\"
DEFINES += VER=\"$${GIT_VERSION}\"

DEFINES += VER1=\\"V1.2.3\\"

TARGET = serial-ninja
TEMPLATE = app
DESTDIR = bin
INCLUDEPATH += libs

OBJECTS_DIR = .generated/
MOC_DIR = .generated/
RCC_DIR = .generated/
UI_DIR = .generated/

SOURCES += main.cpp\
        mainwindow.cpp \
    connectdialog.cpp \
    sessionmanager.cpp \
    outputmanager.cpp \
    historycombobox.cpp \
    history.cpp \
    searchhighlighter.cpp \
    xmodemtransfer.cpp \
    filetransfer.cpp \
    libs/crc16.cpp \
    libs/xmodem.cpp \
    settings.cpp \
    completerinput.cpp

HEADERS  += mainwindow.h \
    connectdialog.h \
    sessionmanager.h \
    outputmanager.h \
    historycombobox.h \
    history.h \
    searchhighlighter.h \
    xmodemtransfer.h \
    filetransfer.h \
    libs/crc16.h \
    libs/xmodem.h \
    settings.h \
    completerinput.h

FORMS    += mainwindow.ui \
    connectdialog.ui \
    searchwidget.ui

RESOURCES += \
    serial-ninja.qrc
