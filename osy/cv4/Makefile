##############################################################################
#
# Zastresujici Makefile ctvrteho cviceni
#
#     make          ... prelozi kalendar1, kalendar2, kalendar3
#     make clean    ... uklidi .o a spustitelne soubory
#
##############################################################################

DIRS = kalendar

.PHONY: all clean $(DIRS)

all: $(DIRS)

$(DIRS):
	$(MAKE) -C $@

clean:
	for d in $(DIRS); do $(MAKE) -C $$d clean; done
