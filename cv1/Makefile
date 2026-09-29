##############################################################################
#
# Zastresujici Makefile celeho projektu
#
#     make          ... prelozi prvni cviceni (knihovny) i druhe cviceni (cv2)
#     make clean    ... uklidi vsechny .o, .a, .so a spustitelne soubory
#     make demo     ... prelozi vse a spusti ukazkovy scenar prvniho cviceni
#
# Poradi je dulezite: programy potrebuji ke slinkovani hotove knihovny.
#
##############################################################################

DIRS = static_lib main1 dyn_lib1 dyn_lib2 main2

.PHONY: all clean demo cv2 $(DIRS)

all: $(DIRS) cv2

static_lib dyn_lib1 dyn_lib2:
	$(MAKE) -C $@

main1: static_lib
	$(MAKE) -C $@

main2: dyn_lib1 dyn_lib2
	$(MAKE) -C $@

# Druhe cviceni: myls, gennum, monitor
cv2:
	$(MAKE) -C cv2

demo: all
	./run_demo.sh

clean:
	for d in $(DIRS); do $(MAKE) -C $$d clean; done
	$(MAKE) -C cv2 clean
	rm -rf demo_out
