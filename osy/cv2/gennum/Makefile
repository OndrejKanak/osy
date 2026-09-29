##############################################################################
#
# gennum - generator cisel bezici az do Ctrl-C
#
# Vychazi primo z demo Makefile (Petr Olivka, VSB-TUO): prelozi kazdy *.cpp
# v tomto adresari na samostatny spustitelny program.
#
##############################################################################

SOURCES = $(wildcard *.cpp)
OBJS    = $(SOURCES:%.cpp=%.o)
TARGETS = $(SOURCES:%.cpp=%)

CPPFLAGS += -g -pthread -Wall -Werror
LDFLAGS  += -g -pthread
LDLIBS   += -lrt

.PHONY: all clean

all: $(TARGETS)

$(TARGETS): %: %.o
	g++ $(LDFLAGS) $^ $(LDLIBS) -o $@

clean:
	rm -rf *.o $(TARGETS)
