QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    ../src/core.cpp \
    main.cpp \
    miner.cpp \
    widget.cpp

HEADERS += \
    ../src/core.h \
    ../src/version.h \
    configure.h \
    miner.h \
    widget.h \

FORMS += \
    widget.ui

QMAKE_CXXFLAGS += -O3

RESOURCES += \
    resources.qrc

LIBS += \
    -lsodium \
    -lpthread \

win32 {
LIBS += \
    -lws2_32

RC_FILE += ../src/windows/resource.rc
OTHER_FILES += ../src/windows/resource.rc
}
