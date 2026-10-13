SOURCES = $(wildcard *.cpp)
OBJS    = $(SOURCES:%.cpp=%.o)
TARGETS = $(SOURCES:%.cpp=%)

CPPFLAGS += -g -Wall -Werror
LDFLAGS  += -g

.PHONY: all clean

all: $(TARGETS)

$(TARGETS): %: %.o
	g++ $(LDFLAGS) $^ $(LDLIBS) -o $@

clean:
	rm -rf *.o $(TARGETS)
