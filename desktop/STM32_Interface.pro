#-------------------------------------------------
# STM32 Interface - Architecture MVC
# IMT Atlantique
#-------------------------------------------------

QT       += core gui widgets serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = STM32_Interface
TEMPLATE = app

CONFIG += c++17

DEFINES += QT_DEPRECATED_WARNINGS

#-------------------------------------------------
# SOURCES
#-------------------------------------------------
SOURCES += \
    src/main.cpp \
    src/model/DeviceState.cpp \
    src/model/DataModel.cpp \
    src/view/MainWindow.cpp \
    src/controller/DeviceController.cpp \
    src/communication/SerialManager.cpp \
    src/communication/SerialWorker.cpp \
    src/communication/JsonProtocol.cpp

#-------------------------------------------------
# HEADERS
#-------------------------------------------------
HEADERS += \
    src/model/DeviceState.h \
    src/model/DataModel.h \
    src/view/MainWindow.h \
    src/controller/DeviceController.h \
    src/communication/SerialManager.h \
    src/communication/SerialWorker.h \
    src/communication/JsonProtocol.h

#-------------------------------------------------
# FORMS
#-------------------------------------------------
FORMS += \
    src/view/MainWindow.ui

#-------------------------------------------------
# INCLUDE PATHS
#-------------------------------------------------
INCLUDEPATH += \
    src \
    src/model \
    src/view \
    src/controller \
    src/communication

#-------------------------------------------------
# DEPLOYMENT
#-------------------------------------------------
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
