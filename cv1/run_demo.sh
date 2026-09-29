#!/bin/bash
##############################################################################
#
# run_demo.sh - projede cely scenar ze zadani a ukaze, co se deje
#
# Spusteni:   ./run_demo.sh          (nebo: make demo)
# Kdyby skript nesel spustit:   chmod +x run_demo.sh
#
##############################################################################

cd "$( dirname "$0" )" || exit 1

BASE=$PWD
OUT=$BASE/demo_out

LIB1=$BASE/dyn_lib1      # varianta DOPOCET
LIB2=$BASE/dyn_lib2      # varianta KONTROLA

hlavicka()
{
    echo
    echo "============================================================"
    echo "  $*"
    echo "============================================================"
}

# ---------------------------------------------------------------------------
hlavicka "1) Preklad celeho projektu"
# ---------------------------------------------------------------------------
make -s || { echo "PREKLAD SELHAL"; exit 1; }
mkdir -p "$OUT"
echo "Prelozeno."

# ---------------------------------------------------------------------------
hlavicka "2) main1 vygeneruje 8 radku po 5 cislech"
# ---------------------------------------------------------------------------
./main1/main1 8 5 > "$OUT/soubor1.txt"
cat "$OUT/soubor1.txt"

# ---------------------------------------------------------------------------
hlavicka "3) main2 BEZ nastavene LD_LIBRARY_PATH - musi selhat"
# ---------------------------------------------------------------------------
echo "Program zna jen JMENO knihovny, ne cestu k ni:"
env -u LD_LIBRARY_PATH ./main2/main2 < "$OUT/soubor1.txt"
echo "(navratovy kod: $?  -  presne tohle je duvod, proc se LD_LIBRARY_PATH nastavuje)"

# ---------------------------------------------------------------------------
hlavicka "4) dyn_lib1 = DOPOCET souctu  ->  soubor2.txt"
# ---------------------------------------------------------------------------
echo "LD_LIBRARY_PATH=$LIB1"
LD_LIBRARY_PATH=$LIB1 ./main2/main2 < "$OUT/soubor1.txt" > "$OUT/soubor2.txt"
cat "$OUT/soubor2.txt"

# ---------------------------------------------------------------------------
hlavicka "5) dyn_lib2 = KONTROLA souctu nad soubor2.txt - musi projit"
# ---------------------------------------------------------------------------
echo "LD_LIBRARY_PATH=$LIB2"
LD_LIBRARY_PATH=$LIB2 ./main2/main2 < "$OUT/soubor2.txt"
echo "(navratovy kod: $?)"

# ---------------------------------------------------------------------------
hlavicka "6) KONTROLA nad soubor1.txt (bez souctu) - musi hlasit chyby"
# ---------------------------------------------------------------------------
LD_LIBRARY_PATH=$LIB2 ./main2/main2 < "$OUT/soubor1.txt"
echo "(navratovy kod: $?)"

# ---------------------------------------------------------------------------
hlavicka "7) Zamerne poskozeny soucet na 3. radku"
# ---------------------------------------------------------------------------
awk 'NR==3 { $NF = $NF + 42 } { print }' "$OUT/soubor2.txt" > "$OUT/soubor2_vadny.txt"
echo "Radek 3 puvodne: $( sed -n 3p "$OUT/soubor2.txt" )"
echo "Radek 3 rozbity: $( sed -n 3p "$OUT/soubor2_vadny.txt" )"
echo
LD_LIBRARY_PATH=$LIB2 ./main2/main2 < "$OUT/soubor2_vadny.txt"
echo "(navratovy kod: $?)"

# ---------------------------------------------------------------------------
hlavicka "8) Roura: main1 | main2 (dopocet)  ->  soubor3.txt"
# ---------------------------------------------------------------------------
./main1/main1 6 4 | LD_LIBRARY_PATH=$LIB1 ./main2/main2 > "$OUT/soubor3.txt"
cat "$OUT/soubor3.txt"
echo
echo "a hned kontrola teze roury:"
LD_LIBRARY_PATH=$LIB2 ./main2/main2 < "$OUT/soubor3.txt"

# ---------------------------------------------------------------------------
hlavicka "9) Cteni ze souboru argumentem misto presmerovani"
# ---------------------------------------------------------------------------
LD_LIBRARY_PATH=$LIB2 ./main2/main2 "$OUT/soubor3.txt"

# ---------------------------------------------------------------------------
hlavicka "10) Co je uvnitr - podklady k obhajobe"
# ---------------------------------------------------------------------------
echo "--- obsah staticke knihovny (ar t) ---"
ar t static_lib/libgenerator.a

echo
echo "--- symboly ve staticke knihovne (nm) ---"
nm static_lib/libgenerator.a | grep " T " || true

echo
echo "--- typ obou souboru knihoven (file) ---"
file static_lib/libgenerator.a dyn_lib1/libcompute.so dyn_lib2/libcompute.so

echo
echo "--- main1 nema zadnou zavislost na nasi knihovne (staticka!) ---"
ldd main1/main1

echo
echo "--- main2 POTREBUJE libcompute.so, ale nevi kde je ---"
readelf -d main2/main2 | grep -E "NEEDED|RPATH|RUNPATH"
echo
echo "ldd BEZ LD_LIBRARY_PATH:"
env -u LD_LIBRARY_PATH ldd main2/main2 | grep compute
echo "ldd S LD_LIBRARY_PATH=dyn_lib1:"
LD_LIBRARY_PATH=$LIB1 ldd main2/main2 | grep compute
echo "ldd S LD_LIBRARY_PATH=dyn_lib2:"
LD_LIBRARY_PATH=$LIB2 ldd main2/main2 | grep compute

echo
echo "--- exportovana funkce compute v obou knihovnach (nm -D) ---"
echo "dyn_lib1:"; nm -D --defined-only dyn_lib1/libcompute.so | grep compute
echo "dyn_lib2:"; nm -D --defined-only dyn_lib2/libcompute.so | grep compute
echo "(stejny nazev symbolu = knihovny jsou zamenitelne)"

hlavicka "HOTOVO - vystupni soubory najdes v demo_out/"
ls -l "$OUT"
