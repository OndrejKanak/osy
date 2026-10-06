#!/bin/bash
##############################################################################
#
# run_demo.sh - projede ukazkovy scenar tretiho cviceni
#
# Spusteni:   ./run_demo.sh          (nebo: make demo)
#
# Vse bezi v jednom terminalu: stdout myls jde do demo_out/vypis.txt,
# stderr do demo_out/zmeny.txt. Pri obhajobe je nazornejsi pustit myls
# primo a stderr poslat do druheho terminalu - viz README.md.
#
##############################################################################

cd "$( dirname "$0" )" || exit 1

OUT=$PWD/demo_out

hlavicka()
{
    echo
    echo "============================================================"
    echo "  $*"
    echo "============================================================"
}

hlavicka "1) Preklad"
make -s || { echo "PREKLAD SELHAL"; exit 1; }

rm -rf "$OUT"
mkdir -p "$OUT"
cd "$OUT" || exit 1

printf 'prvni soubor\n'           > a.txt
printf 'druhy, o neco delsi\n'    > b.txt
printf 'c\n'                      > c.txt

hlavicka "2) Spoustim: myls -S -u -s -r -t a.txt b.txt c.txt nic.txt"
../myls/myls -S -u -s -r -t a.txt b.txt c.txt nic.txt > vypis.txt 2> zmeny.txt &
LS=$!
sleep 1

echo "+ echo 'pridany radek' >> a.txt";  echo 'pridany radek' >> a.txt;  sleep 3
echo "+ rm c.txt";                       rm c.txt;                       sleep 3
echo "+ printf 'bez odradkovani' >> b.txt"
printf 'bez odradkovani' >> b.txt;                                       sleep 3
echo "+ echo 'c je zpet' > c.txt";       echo 'c je zpet' > c.txt;       sleep 3
echo "+ chmod 000 b.txt";                chmod 000 b.txt;                sleep 3
echo "+ truncate -s 3 a.txt";            truncate -s 3 a.txt;            sleep 3

kill -INT $LS
wait $LS 2>/dev/null
chmod 644 b.txt

hlavicka "3) Co myls vypisoval (stdout)"
cat vypis.txt

hlavicka "4) Co myls hlasil na stderr"
cat zmeny.txt

if [ "$( id -u )" = 0 ]; then
    echo
    echo "POZOR: bezi jako root - root smi cist vse, proto se u b.txt"
    echo "neobjevi  <-- NELZE CIST. Spust demo jako bezny uzivatel."
fi
