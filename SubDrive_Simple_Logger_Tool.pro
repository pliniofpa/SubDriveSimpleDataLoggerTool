#-------------------------------------------------
#
# Project created by QtCreator 2014-09-05T09:49:14
#
#-------------------------------------------------

QT       += core gui xml network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SubDrive_Simple_Logger_Tool
TEMPLATE = app

win32:RC_FILE = resources.rc

SOURCES += main.cpp\
        mainwindow.cpp \
    aboutdialog.cpp \
    networkconfigdialog.cpp \
    smtp/smtp.cpp \
    emailconfig.cpp

HEADERS  += \
    aboutdialog.h \
    version.h \
    networkconfigdialog.h \
    smtp/smtp.h \
    emailconfig.h \
    mainwindow.h

win32:HEADERS += 

win32:INCLUDEPATH += "C:\Qt_Projects\SubDriveDataLogger"

FORMS    += mainwindow.ui \
    aboutdialog.ui \
    networkconfigdialog.ui \
    emailconfig.ui

CONFIG += mobility
MOBILITY =

RESOURCES += \
    res.qrc

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

OTHER_FILES += \
    android/AndroidManifest.xml

