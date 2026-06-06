TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main.cpp \
        parameters.cpp

LIBS += \
        -lsodium \
        -lpthread
win32 {
    LIBS += -lws2_32
}

QMAKE_CXXFLAGS += \
        -O3

HEADERS += \
        configure.h \
        cppcodec/base32_rfc4648.hpp \
        cppcodec/data/access.hpp \
        cppcodec/data/raw_result_buffer.hpp \
        cppcodec/detail/base32.hpp \
        cppcodec/detail/codec.hpp \
        cppcodec/detail/config.hpp \
        cppcodec/detail/stream_codec.hpp \
        cppcodec/parse_error.hpp \
        main.h \
        parametes.h
