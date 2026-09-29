##############################################################################
#
# dyn_lib2 - preklad DYNAMICKE (sdilene) knihovny libcompute.so
#
# Varianta KONTROLA - overi, ze posledni cislo na radku je souctem predchozich.
#
# Obe knihovny (dyn_lib1 i dyn_lib2) se jmenuji STEJNE a maji STEJNOU funkci
# compute(). Lisi se jen obsahem. Proto je mozne pri spusteni programu main2
# vybrat pomoci LD_LIBRARY_PATH, ktera z nich se skutecne pouzije.
#
# Dva prepinace navic oproti beznemu prekladu:
#   -fPIC     kod nezavisly na poloze v pameti (nutne pro sdilene knihovny)
#   -shared   linker ma vyrobit sdilenou knihovnu, ne spustitelny program
#
##############################################################################

LIB     = libcompute.so

SOURCES = $(wildcard *.cpp)
OBJS    = $(SOURCES:%.cpp=%.o)

CPPFLAGS += -g -pthread -Wall -Werror -fPIC
LDFLAGS  += -g -pthread -shared

.PHONY: all clean

all: $(LIB)

$(LIB): $(OBJS)
	g++ $(LDFLAGS) $^ -o $@

clean:
	rm -rf *.o $(LIB)
