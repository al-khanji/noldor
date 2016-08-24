TEMPLATE = app
CONFIG += console c++14
CONFIG -= app_bundle qt

DEFINES += BUILDING_NOLDOR

SOURCES += main.cpp \
    memory.cpp \
    util.cpp \
    types.cpp \
    parser.cpp \
    interpreter.cpp
# \
#    vm.cpp

HEADERS += \
    noldor.h \
    noldor_impl.h
