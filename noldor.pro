TEMPLATE = app
CONFIG += console c++14
CONFIG -= app_bundle qt

DEFINES += BUILDING_NOLDOR

SOURCES += main.cpp \
    memory.cpp \
    util.cpp \
    parser.cpp \
    interpreter.cpp \
    types/cons.cpp \
    types/string.cpp \
    types/symbol.cpp \
    types/bool.cpp \
    types/number.cpp \
    types/environment.cpp \
    types/vector.cpp \
    types/char.cpp \
    types/procedure.cpp \
    types/eof_object.cpp \
    types/macro.cpp
# \
#    vm.cpp

HEADERS += \
    noldor.h \
    noldor_impl.h
