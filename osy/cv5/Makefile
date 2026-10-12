##############################################################################
#
# Zastresujici Makefile pateho cviceni
#
#     make          ... prelozi kolona1, kolona2, kolona3
#     make clean    ... uklidi .o a spustitelne soubory
#
##############################################################################

DIRS = kolona

.PHONY: all clean $(DIRS)

all: $(DIRS)

$(DIRS):
	$(MAKE) -C $@

clean:
	for d in $(DIRS); do $(MAKE) -C $$d clean; done
