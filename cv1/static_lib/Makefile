##############################################################################
#
# static_lib - preklad STATICKE knihovny libgenerator.a
#
# Vychazi z demo Makefile (Petr Olivka, VSB-TUO).
#
# Staticka knihovna je jen archiv prelozenych .o souboru, ktery vyrobi
# program 'ar':
#     r = vloz/nahrad soubory v archivu
#     c = archiv vytvor, kdyz jeste neexistuje (bez varovani)
#     s = vytvor index symbolu (drive samostatny prikaz 'ranlib')
#
##############################################################################

LIB     = libgenerator.a

SOURCES = $(wildcard *.cpp)
OBJS    = $(SOURCES:%.cpp=%.o)

CPPFLAGS += -g -pthread -Wall -Werror

.PHONY: all clean

all: $(LIB)

# Implicitni pravidlo make prelozi kazdy .cpp na .o pomoci
#     $(CXX) $(CXXFLAGS) $(CPPFLAGS) -c
$(LIB): $(OBJS)
	ar rcs $@ $^

clean:
	rm -rf *.o $(LIB)
