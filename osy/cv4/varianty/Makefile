##############################################################################
#
# varianty - mozna rozsireni pripravy kalendar
#
# Stejny Makefile jako v kalendar/: kazdy *.cpp je samostatny program.
# Seznam svatku se bere z ../kalendar/svatky.hpp (prepinac -I).
#
##############################################################################

SOURCES = $(wildcard *.cpp)
OBJS    = $(SOURCES:%.cpp=%.o)
TARGETS = $(SOURCES:%.cpp=%)

CPPFLAGS += -g -pthread -Wall -Werror -I../kalendar
LDFLAGS  += -g -pthread
LDLIBS   += -lrt

.PHONY: all clean

all: $(TARGETS)

$(TARGETS): %: %.o
	g++ $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJS): spolecne.hpp ../kalendar/svatky.hpp

clean:
	rm -rf *.o $(TARGETS)
