QT += core sql network serialbus

QT -= gui
CONFIG +=  console
CONFIG -= app_bundle
TEMPLATE = app
VERSION = "1.1"
TARGET = regulatorshtyl
target.path = /home/pi/regulator # путь на устройстве
INSTALLS += target


#include( common.pri )

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.

#DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        main.cpp \
    ups_status.cpp \
    service.cpp \
    processor.cpp \
    meteotcpsock.cpp \
    ivtm.cpp




HEADERS += \
    app.h \
    service.h \
    processor.h \
    ups_status.h \
    processor.h \
    meteotcpsock.h \
    ivtm.h

LIBS += -lnetsnmp -ltelnet -lnetsnmpmibs

DEFINES += APP_VERSION=$$VERSION
