##############################################################################
#
# Zastresujici Makefile ctvrteho cviceni
#
#     make          ... prelozi kalendar1-3 a vsechny varianty
#     make clean    ... uklidi .o a spustitelne soubory
#
##############################################################################

DIRS = kalendar varianty

.PHONY: all clean $(DIRS)

all: $(DIRS)

$(DIRS):
	$(MAKE) -C $@

clean:
	for d in $(DIRS); do $(MAKE) -C $$d clean; done
