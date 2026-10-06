##############################################################################
#
# kalendar - procesy a roury, tri postupne verze
#
# Vychazi primo z demo Makefile (Petr Olivka, VSB-TUO): prelozi kazdy *.cpp
# v tomto adresari na samostatny spustitelny program
# (kalendar1, kalendar2, kalendar3). Vsechny sdileji svatky.hpp.
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

# Po zmene svatky.hpp se musi prelozit vsechny verze.
$(OBJS): svatky.hpp

clean:
	rm -rf *.o $(TARGETS)
