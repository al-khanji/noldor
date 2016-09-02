LIBRARY_SOURCES := \
	interpreter.cpp \
	memory.cpp \
	util.cpp \
	types/bool.cpp \
	types/char.cpp \
	types/cons.cpp \
	types/environment.cpp \
	types/eof_object.cpp \
	types/macro.cpp \
	types/number.cpp \
	types/procedure.cpp \
	types/string.cpp \
	types/symbol.cpp \
	types/vector.cpp \
	types/port.cpp
LIBRARY_OBJECTS := \
	$(LIBRARY_SOURCES:.cpp=.o)
LIBRARY_HEADERS := \
	noldor.h \
	noldor_impl.h

NOLDOR_SOURCES := noldor.cpp
NOLDOR_OBJECTS := $(NOLDOR_SOURCES:.cpp=.o)

NOLDOR_TEST_SOURCES := noldor_tests.cpp
NOLDOR_TEST_OBJECTS := $(NOLDOR_TEST_SOURCES:.cpp=.o)

DEPS := $(LIBRARY_SOURCES:.cpp=.d) $(NOLDOR_SOURCES:.cpp=.d) $(NOLDOR_TEST_SOURCES:.cpp=.d)
CXXFLAGS += -std=c++14 -Wall -Wextra -pedantic -Werror -I. -MMD -MP -g

INSTALL_PREFIX ?= /usr/local
bindir ?= $(INSTALL_PREFIX)/bin

noldor: $(LIBRARY_OBJECTS) $(NOLDOR_OBJECTS)
	$(CXX) -o $@ $(LIBRARY_OBJECTS) $(NOLDOR_OBJECTS) $(LDFLAGS)

noldor_test: $(LIBRARY_OBJECTS) $(NOLDOR_TEST_OBJECTS)
	$(CXX) -o $@ $(LIBRARY_OBJECTS) $(NOLDOR_TEST_OBJECTS) $(LDFLAGS)

all: check noldor

install: noldor
	install -d $(bindir)
	install -s noldor $(bindir)

check: noldor_test
	./noldor_test

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f noldor
	rm -f noldor_test
	rm -f $(LIBRARY_OBJECTS)
	rm -f $(NOLDOR_OBJECTS)
	rm -f $(NOLDOR_TEST_OBJECTS)
	rm -f $(DEPS)

-include $(DEPS)
