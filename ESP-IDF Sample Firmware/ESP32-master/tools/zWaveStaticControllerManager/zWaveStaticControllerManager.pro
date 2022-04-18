#-------------------------------------------------
#
# Project created by QtCreator 2018-01-11T16:26:24
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets network

TARGET = zWaveStaticControllerManager
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += \
        ../../firmware/components/common/include \
        ../../firmware/components/storage/include \
        ../../firmware/components/zwave_gateway/include \
        ../../lib/ZNet/project \
        ../../lib/ZNet/project/datastructs \
        ../../lib/ZNet/project/lib \
        ../../lib/ZNet/project/multitasking \
        ../../lib/ZNet/project/system_time \
        ../../lib/ZNet/project/serial_port \
        ../../lib/esp-idf/components/lwip/include/lwip/apps/sntp \
        ../../lib/esp-idf/components/lwip/include/lwip \
        ../../lib/esp-idf/components/lwip/include/lwip/port \
        ../../lib/ZNet/project/libs2

SOURCES += \
        main.cpp \
        mainwindow.cpp \
        DevIO.cpp \
        SocketIO.cpp \
        DummyZwaveController.cpp \
        ../../firmware/components/zwave_gateway/src/StateFlow.cpp \
        ../../firmware/components/zwave_gateway/src/ZwFaultState.cpp \
        ../../firmware/components/zwave_gateway/src/ZwInitState.cpp \
        ../../firmware/components/zwave_gateway/src/ZwRunState.cpp


HEADERS += \
        mainwindow.h \
        DevIO.h \
        SocketIO.h \
        DummyZwaveController.h \
        ../../firmware/components/common/include/StatesTable.h \
        ../../firmware/components/common/include/Array.h \
        ../../firmware/components/common/include/LockKeeper.h \
        ../../firmware/components/common/include/Mutex.h \
        ../../firmware/components/common/include/PointerArray.h \
        ../../firmware/components/common/include/State.h \
        ../../firmware/components/common/include/StateMachine.h \
        ../../firmware/components/common/include/StatesTable.h \
        ../../firmware/components/common/include/TransitionsTable.h \
        ../../firmware/components/storage/include/Storage.h \
        ../../firmware/components/zwave_gateway/include/StateFlow.h \
        ../../firmware/components/zwave_gateway/include/ZwFaultState.h \
        ../../firmware/components/zwave_gateway/include/ZwInitState.h \
        ../../firmware/components/zwave_gateway/include/ZwRunState.h

FORMS += \
        mainwindow.ui
