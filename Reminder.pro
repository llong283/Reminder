#-------------------------------------------------
#
# Project created by QtCreator 2011-12-09T19:19:34
#
#-------------------------------------------------

QT += core gui sql

TARGET = Reminder
TEMPLATE = app

RC_FILE = icon.rc

SOURCES += main.cpp\
        mainwindow.cpp \
    SolarDate.cpp \
    ChineseDate.cpp \
    ChineseCalendarDB.cpp \
    dialogset.cpp \
    dialogremind.cpp

HEADERS  += mainwindow.h \
    SolarDate.h \
    ChineseDate.h \
    ChineseCalendarDB.h \
    dialogset.h \
    dialogremind.h

FORMS    += mainwindow.ui \
    dialogset.ui \
    dialogremind.ui

RESOURCES += \
    resource.qrc

OTHER_FILES += \
    icon.rc
