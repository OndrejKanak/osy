#!/bin/bash
##############################################################################
#
# run_demo.sh - projede ukazkovy scenar druheho cviceni
#
# Spusteni:   ./run_demo.sh          (nebo: make demo)
# Kdyby skript nesel spustit:   chmod +x run_demo.sh
#
# Scenar generatoru a monitoru se tu odehrava v jednom terminalu na pozadi.
# Pri obhajobe je nazornejsi pustit si kazdy program ve vlastnim terminalu -
# viz README.md.
#
##############################################################################

cd "$( dirname "$0" )" || exit 1

BASE=$PWD
OUT=$BASE/demo_out

hlavicka()
{
    echo
    echo "============================================================"
    echo "  $*"
    echo "============================================================"
}

# ---------------------------------------------------------------------------
hlavicka "1) Preklad"
# ---------------------------------------------------------------------------
make -s || { echo "PREKLAD SELHAL"; exit 1; }
mkdir -p "$OUT"
echo "Prelozeno: myls, gennum, monitor"

# ===========================================================================
#   MYLS
# ===========================================================================

# ---------------------------------------------------------------------------
hlavicka "2) myls bez prepinacu - jen jmena"
# ---------------------------------------------------------------------------
./myls/myls myls/myls.cpp gennum/gennum.cpp monitor/monitor.cpp

# ---------------------------------------------------------------------------
hlavicka "3) myls -s  (velikost)"
# ---------------------------------------------------------------------------
./myls/myls -s myls/myls.cpp gennum/gennum.cpp monitor/monitor.cpp

# ---------------------------------------------------------------------------
hlavicka "4) PORADI PREPINACU urcuje poradi sloupcu"
# ---------------------------------------------------------------------------
echo "--- ./myls -s -t -r myls/myls.cpp ---"
./myls/myls -s -t -r myls/myls.cpp
echo
echo "--- ./myls -r -t -s myls/myls.cpp   (obracene) ---"
./myls/myls -r -t -s myls/myls.cpp
echo
echo "--- ./myls -t myls/myls.cpp ---"
./myls/myls -t myls/myls.cpp

# ---------------------------------------------------------------------------
hlavicka "5) myls na sadu souboru rozvinutou shellem"
# ---------------------------------------------------------------------------
echo "--- ./myls -r -s */*.cpp ---"
./myls/myls -r -s */*.cpp
echo
echo "--- ./myls -r -s /etc/*.conf  (prvnich 8 radku) ---"
./myls/myls -r -s /etc/*.conf 2>/dev/null | head -8

# ---------------------------------------------------------------------------
hlavicka "6) myls na ruzne typy souboru - vsimni si prvniho znaku prav"
# ---------------------------------------------------------------------------
./myls/myls -r . /etc /etc/passwd /dev/null /dev/sda 2>/dev/null

# ---------------------------------------------------------------------------
hlavicka "7) Nenalezene soubory se vypisi v souhrnu na konci"
# ---------------------------------------------------------------------------
./myls/myls -s myls/myls.cpp tohle_neexistuje.txt gennum/gennum.cpp ani_tohle.c
echo "(navratovy kod: $?)"

# ===========================================================================
#   GENNUM + MONITOR
# ===========================================================================

# ---------------------------------------------------------------------------
hlavicka "8) gennum - generuje, dokud ho nezastavime"
# ---------------------------------------------------------------------------
DATA=$OUT/out.txt
: > "$DATA"

echo "spoustim: ./gennum/gennum 6 120 > demo_out/out.txt   (1 radek za 0,5 s)"
./gennum/gennum 6 120 > "$DATA" 2>/dev/null &
GEN=$!
sleep 3

echo
echo "po 3 sekundach ma soubor $( wc -c < "$DATA" ) B / $( wc -l < "$DATA" ) radku:"
cat "$DATA"
echo
echo "Nenulova velikost je dukaz, ze v gennum funguje fflush( stdout )."
echo "Bez nej by se do souboru nic nezapsalo, dokud se nenaplni 4 kB buffer."

# ---------------------------------------------------------------------------
hlavicka "9) monitor - sleduje soubor jako tail -f"
# ---------------------------------------------------------------------------
echo "spoustim: ./monitor/monitor demo_out/out.txt"
echo "(monitor zacina na konci souboru, takze vypise jen to, co nove pribude)"
echo
./monitor/monitor "$DATA" > "$OUT/monitor.log" 2> "$OUT/monitor.err" &
MON=$!
sleep 4

echo "monitor zatim vypsal $( wc -l < "$OUT/monitor.log" ) radku."

# ---------------------------------------------------------------------------
hlavicka "10) Zastaveni generatoru a zkraceni souboru"
# ---------------------------------------------------------------------------
echo "zastavuji generator (Ctrl-C = SIGINT)..."
kill -INT $GEN 2>/dev/null
wait $GEN 2>/dev/null
echo "hotovo, soubor ma $( wc -c < "$DATA" ) B"
sleep 2

echo
echo "truncate -s 0 demo_out/out.txt"
truncate -s 0 "$DATA"
sleep 3

echo
echo "Generator je schvalne zastaveny PRED zkracenim. Kdyby bezel dal,"
echo "dopsal by data na svou puvodni pozici a soubor by se hned zvetsil"
echo "zpatky - monitor by pri kontrole 1x za sekundu zkraceni nemusel"
echo "vubec zahlednout."

# ---------------------------------------------------------------------------
hlavicka "11) Opetovne spusteni generatoru"
# ---------------------------------------------------------------------------
./gennum/gennum 6 180 >> "$DATA" 2>/dev/null &
GEN=$!
sleep 4
kill -INT $GEN 2>/dev/null
wait $GEN 2>/dev/null
sleep 2

kill -INT $MON 2>/dev/null
wait $MON 2>/dev/null

# ---------------------------------------------------------------------------
hlavicka "12) Co monitor hlasil (jeho stderr)"
# ---------------------------------------------------------------------------
cat "$OUT/monitor.err"

# ---------------------------------------------------------------------------
hlavicka "13) Co monitor vypsal (jeho stdout)"
# ---------------------------------------------------------------------------
cat "$OUT/monitor.log"

hlavicka "HOTOVO - soubory najdes v demo_out/"
ls -l "$OUT"
