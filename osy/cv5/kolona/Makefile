##############################################################################
#
# kolona - exec a dup2, tri postupne verze
#
# Vychazi primo z demo Makefile (Petr Olivka, VSB-TUO): prelozi kazdy *.cpp
# v tomto adresari na samostatny spustitelny program
# (kolona1, kolona2, kolona3).
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
