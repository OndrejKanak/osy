##############################################################################
#
# main1 - program, ktery generuje M radku po N cislech
#
# Linkuje se se STATICKOU knihovnou libgenerator.a z adresare ../static_lib.
#
# Pouzite prepinace (vic jich zadani nedovoluje a vic jich netreba):
#   -I../static_lib   kde hledat hlavickovy soubor generator.h
#   -L../static_lib   kde hledat soubor knihovny
#   -lgenerator       ktera knihovna - jmeno BEZ predpony 'lib' a BEZ pripony
#                     (linker si sam slozi nazev libgenerator.a)
#
# Protoze jde o statickou knihovnu, vykopiruje se pouzity kod primo do
# programu main1. Vysledny program uz za behu zadnou knihovnu nepotrebuje.
#
##############################################################################

TARGET  = main1

SOURCES = $(wildcard *.cpp)
OBJS    = $(SOURCES:%.cpp=%.o)

LIBDIR  = ../static_lib
LIBNAME = generator
LIBFILE = $(LIBDIR)/lib$(LIBNAME).a

CPPFLAGS += -g -pthread -Wall -Werror -I$(LIBDIR)
LDFLAGS  += -g -pthread -L$(LIBDIR)
LDLIBS   += -l$(LIBNAME) -lrt

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS) $(LIBFILE)
	g++ $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@

# Kdyby knihovna jeste nebyla prelozena, prelozi se sama.
$(LIBFILE):
	$(MAKE) -C $(LIBDIR)

clean:
	rm -rf *.o $(TARGET)
