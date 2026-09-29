##############################################################################
#
# Zastresujici Makefile tretiho cviceni
#
#     make          ... prelozi myls
#     make clean    ... uklidi .o a spustitelne soubory
#     make demo     ... prelozi a spusti ukazkovy scenar (run_demo.sh)
#
##############################################################################

DIRS = myls

.PHONY: all clean demo $(DIRS)

all: $(DIRS)

$(DIRS):
	$(MAKE) -C $@

demo: all
	./run_demo.sh

clean:
	for d in $(DIRS); do $(MAKE) -C $$d clean; done
	rm -rf demo_out
