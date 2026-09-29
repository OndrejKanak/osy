##############################################################################
#
# main2 - program, ktery preda vstup funkci compute() z DYNAMICKE knihovny
#
# Program se preklada vzdy jen s JEDNOU knihovnou libcompute.so:
#
#     make                      ... slinkuje proti ../dyn_lib1
#     make LIBDIR=../dyn_lib2   ... slinkuje proti ../dyn_lib2
#
# Na vysledku ale prakticky nezalezi! Do programu se ulozi jen JMENO
# potrebne knihovny (libcompute.so), nikoli cesta k ni. Overit se to da:
#
#     readelf -d main2 | grep NEEDED
#
# Kterou knihovnu program pri spusteni skutecne nacte, urcuje az dynamicky
# linker (ld.so) podle promenne prostredi LD_LIBRARY_PATH:
#
#     LD_LIBRARY_PATH=../dyn_lib1 ./main2 < data.txt > data_soucty.txt
#     LD_LIBRARY_PATH=../dyn_lib2 ./main2 < data_soucty.txt
#
# Pouzite prepinace:
#   -I$(LIBDIR)   kde hledat hlavicku compute.h (v obou adresarich stejna)
#   -L$(LIBDIR)   kde hledat knihovnu PRI PREKLADU
#   -lcompute     ktera knihovna - linker si slozi nazev libcompute.so
#
##############################################################################

TARGET  = main2

SOURCES = $(wildcard *.cpp)
OBJS    = $(SOURCES:%.cpp=%.o)

# Knihovnu, se kterou se linkuje, lze zmenit z prikazove radky:
#     make LIBDIR=../dyn_lib2
LIBDIR  ?= ../dyn_lib1
LIBNAME  = compute
LIBFILE  = $(LIBDIR)/lib$(LIBNAME).so

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
