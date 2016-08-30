SOURCES := \
	interpreter.cpp \
	main.cpp \
	memory.cpp \
	parser.cpp \
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
	types/vector.cpp

HEADERS := \
	noldor.h \
	noldor_impl.h

OBJECTS := $(SOURCES:.cpp=.o)
DEPS := $(SOURCES:.cpp=.d)

CXXFLAGS += -std=c++14 -Wall -Wextra -pedantic -Werror -I. -MMD -MP -g

all: noldor

noldor: $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f noldor
	rm -f $(OBJECTS)
	rm -f $(DEPS)

-include $(DEPS)
