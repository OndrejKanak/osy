##############################################################################
#
# Zastresujici Makefile druheho cviceni
#
#     make          ... prelozi myls, gennum i monitor
#     make clean    ... uklidi vsechny .o a spustitelne soubory
#     make demo     ... prelozi vse a spusti ukazkovy scenar (run_demo.sh)
#
##############################################################################

DIRS = myls gennum monitor

.PHONY: all clean demo $(DIRS)

all: $(DIRS)

$(DIRS):
	$(MAKE) -C $@

demo: all
	./run_demo.sh

clean:
	for d in $(DIRS); do $(MAKE) -C $$d clean; done
	rm -rf demo_out
