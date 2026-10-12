##############################################################################
#
# Zastresujici Makefile pateho cviceni
#
#     make          ... prelozi kolona1-3 a vsechny varianty
#     make clean    ... uklidi .o a spustitelne soubory
#
##############################################################################

DIRS = kolona varianty

.PHONY: all clean $(DIRS)

all: $(DIRS)

$(DIRS):
	$(MAKE) -C $@

clean:
	for d in $(DIRS); do $(MAKE) -C $$d clean; done
