##############################################################################
#
# Zastresujici Makefile pateho cviceni
#
#     make          ... prelozi kolona1-3, varianty a obraz
#     make clean    ... uklidi .o a spustitelne soubory
#
##############################################################################

DIRS = kolona varianty obraz

.PHONY: all clean $(DIRS)

all: $(DIRS)

$(DIRS):
	$(MAKE) -C $@

clean:
	for d in $(DIRS); do $(MAKE) -C $$d clean; done
