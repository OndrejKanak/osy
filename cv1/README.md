# Operační systémy — příprava na cvičení

> **Druhé cvičení** (`myls`, `gennum`, `monitor`) je v podadresáři
> [cv2/](cv2/) a má vlastní [README](cv2/README.md) i [OBHAJOBA.md](cv2/OBHAJOBA.md).
> `make` v tomto adresáři přeloží obojí.

---

Projekt obsahuje **dva programy**, **jednu statickou knihovnu** a **dvě dynamické
knihovny se stejným názvem**. Která dynamická knihovna se skutečně použije, se
rozhoduje až při spuštění pomocí proměnné prostředí `LD_LIBRARY_PATH`.

---

## 1. Struktura projektu

```
osy/
├── Makefile              zastřešující překlad všeho ve správném pořadí
├── run_demo.sh           projede celý scénář ze zadání a ukáže, co se děje
├── README.md             tento soubor
├── OBHAJOBA.md           otázky, na které se učitel ptá, i s odpověďmi
│
├── static_lib/           STATICKÁ knihovna libgenerator.a
│   ├── generator.h           rozhraní: gen_init, gen_number, gen_line, gen_matrix
│   ├── generator.cpp         generování čísel 10 až 1000
│   └── Makefile              g++ -c  +  ar rcs
│
├── main1/                PROGRAM main1 — generátor dat
│   ├── main1.cpp             jen funkce main a kontrola argumentů
│   └── Makefile              -I../static_lib  -L../static_lib  -lgenerator
│
├── dyn_lib1/             DYNAMICKÁ knihovna libcompute.so — varianta DOPOČET
│   ├── compute.h             int compute( FILE * )
│   ├── compute.cpp           ke každému řádku připojí součet čísel
│   └── Makefile              g++ -fPIC -c  +  g++ -shared
│
├── dyn_lib2/             DYNAMICKÁ knihovna libcompute.so — varianta KONTROLA
│   ├── compute.h             ÚPLNĚ STEJNÁ hlavička jako v dyn_lib1
│   ├── compute.cpp           ověří, že poslední číslo je součtem předchozích
│   └── Makefile              g++ -fPIC -c  +  g++ -shared
│
└── main2/                PROGRAM main2 — jen předá vstup funkci compute()
    ├── main2.cpp
    └── Makefile              -I../dyn_lib1  -L../dyn_lib1  -lcompute
```

Klíčová myšlenka: **`dyn_lib1/libcompute.so` a `dyn_lib2/libcompute.so` se jmenují
stejně a exportují stejnou funkci `compute()`.** Program `main2` je slinkován jen
s jednou z nich, ale protože se do programu ukládá pouze *jméno* knihovny a ne
cesta k ní, dá se při spuštění podstrčit ta druhá.

---

## 2. Přenos na školní Linux

Soubory jsou uložené s unixovými konci řádků (LF). Kdyby je někdo otevřel
a uložil v poznámkovém bloku na Windows, `make` by zahlásil nesmyslnou chybu —
pak pomůže:

```bash
dos2unix Makefile */Makefile */*.cpp */*.h run_demo.sh
```

Přenos z Windows na server:

```bash
scp -r osy LOGIN@SERVER:~/
```

Na serveru pak:

```bash
cd ~/osy
chmod +x run_demo.sh
make
```

---

## 3. Překlad

```bash
make
```

```bash
make clean
```

```bash
make demo
```

`make` přeloží vše, `make clean` uklidí `.o`, `.a`, `.so` i spustitelné soubory,
`make demo` přeloží vše a spustí ukázkový scénář.

Dá se překládat i po částech, pořadí je dané závislostmi:

```bash
make -C static_lib && make -C main1 && make -C dyn_lib1 && make -C dyn_lib2 && make -C main2
```

`main2` se ve výchozím stavu linkuje proti `../dyn_lib1`. Kdyby ses chtěl přesvědčit,
že na tom nezáleží, přelož ho proti druhé:

```bash
make -C main2 clean && make -C main2 LIBDIR=../dyn_lib2
```

Výsledný program se chová naprosto stejně — viz kapitola 6.

---

## 4. Spuštění — scénář přesně podle zadání

```bash
./main1/main1 100 30 > soubor1.txt
```

```bash
export LD_LIBRARY_PATH=$PWD/dyn_lib1
./main2/main2 < soubor1.txt > soubor2.txt
```

```bash
export LD_LIBRARY_PATH=$PWD/dyn_lib2
./main2/main2 < soubor2.txt
```

Totéž rovnou rourou, bez mezisouboru:

```bash
export LD_LIBRARY_PATH=$PWD/dyn_lib1
./main1/main1 100 30 | ./main2/main2 > soubor3.txt
```

```bash
export LD_LIBRARY_PATH=$PWD/dyn_lib2
./main2/main2 < soubor3.txt
```

Nebo bez trvalé změny prostředí, jen pro jeden příkaz:

```bash
LD_LIBRARY_PATH=$PWD/dyn_lib1 ./main2/main2 < soubor1.txt > soubor2.txt
```

```bash
LD_LIBRARY_PATH=$PWD/dyn_lib2 ./main2/main2 < soubor2.txt
```

### Nejčastější past celého cvičení

Tohle **nefunguje**, i když to tak v zadání na první pohled vypadá:

```
LD_LIBRARY_PATH=dyn_lib1
./main2/main2 < soubor1.txt
```

Samotný zápis `PROMĚNNÁ=hodnota` na vlastním řádku vytvoří jen **proměnnou
shellu**, nikoli **proměnnou prostředí**. Spuštěný proces ji nezdědí, protože
proces dostává při `exec` jen prostředí (`environ`), ne interní proměnné shellu.
Funguje jedině:

- `export LD_LIBRARY_PATH=...` na vlastním řádku (platí pro celou relaci), **nebo**
- `LD_LIBRARY_PATH=... ./main2/main2` na jednom řádku (platí jen pro ten jeden příkaz).

Ověření, že je proměnná opravdu v prostředí:

```bash
env | grep LD_LIBRARY_PATH
```

Druhá věc: v `LD_LIBRARY_PATH` používej raději **absolutní cestu** (`$PWD/dyn_lib1`).
Relativní cesta se vyhodnocuje vůči aktuálnímu adresáři *procesu*, takže po `cd`
jinam přestane platit.

---

## 5. Co dělá který program

### `main1 M N`

Vypíše `M` řádků, na každém `N` náhodných čísel z intervalu 10 až 1000 — tedy
nikdy ne jednociferných. Výstup jde na `stdout`, takže se dá přesměrovat do
souboru nebo poslat rourou.

```bash
./main1/main1 5 4
```

```
731 118 990 267
45 612 88 1000
...
```

Vlastní generování je ve statické knihovně, `main1.cpp` obsahuje jen `main()`
a kontrolu argumentů — přesně jak zadání vyžaduje.

### `main2 [soubor]`

Otevře vstup (bez argumentu čte `stdin`) a předá ho funkci `compute()`
z dynamické knihovny. Sám o sobě nedělá nic jiného.

S knihovnou **dyn_lib1** (dopočet) se řádek `731 118 990 267` změní na
`731 118 990 267 2106`.

S knihovnou **dyn_lib2** (kontrola) dostaneš zprávu typu:

```
Radek 3: spatny soucet - nalezeno 2148, ocekavano 2106.
Kontrola NEPROSLA: vadnych radku 1 z 8.
```

Návratový kód `main2` je `0` při úspěchu, `1` když se našly vadné řádky
a `2` při chybě vstupu. Dá se tedy použít ve skriptu:

```bash
if LD_LIBRARY_PATH=$PWD/dyn_lib2 ./main2/main2 < soubor2.txt > /dev/null; then echo "soubor je v poradku"; fi
```

Kontrola pozná součet tak, že bere **poslední číslo na řádku** jako deklarovaný
součet a porovná ho se součtem všech čísel před ním. Proto řádek bez dopočteného
součtu spolehlivě označí za vadný.

Obě knihovny čtou řádky pomocnou funkcí `read_line()`, která si buffer sama
zvětšuje `realloc()`em — délka řádku tedy není nijak omezená. Při `./main1 100 500`
má řádek přes 1900 znaků a projde bez problémů.

---

## 6. Statická vs. dynamická knihovna — v čem je rozdíl

| | statická `libgenerator.a` | dynamická `libcompute.so` |
|---|---|---|
| co to je | archiv `.o` souborů (jako ZIP) | skoro hotový program bez `main()` |
| vyrobí ji | `ar rcs lib….a *.o` | `g++ -shared *.o -o lib….so` |
| překlad `.o` | `g++ -c` | `g++ -fPIC -c` |
| kdy se použije | **při linkování** | **až při spuštění** |
| kód se | zkopíruje do programu | do programu vůbec nedostane |
| program za běhu | knihovnu nepotřebuje | bez knihovny se vůbec nespustí |
| výměna knihovny | nutný nový překlad programu | stačí vyměnit soubor `.so` |

Právě poslední řádek je pointa cvičení: `main2` se nepřekládá znovu, a přesto
jednou sčítá a podruhé kontroluje.

Ověř si to sám:

```bash
ldd main1/main1
```

O `libgenerator.a` tam není ani zmínka — kód se do programu zkopíroval.

```bash
readelf -d main2/main2 | grep NEEDED
```

Uloženo je jen **jméno** `libcompute.so`, žádná cesta.

---

## 7. Přepínače překladače — jen tři, víc jich netřeba

| přepínač | kdy se uplatní | co dělá |
|---|---|---|
| `-I cesta` | při překladu `.cpp` na `.o` | kde hledat hlavičku z `#include "…"` |
| `-L cesta` | při linkování | v jakém adresáři hledat soubor knihovny |
| `-l jméno` | při linkování | kterou knihovnu použít |

U `-l` se píše **jméno bez předpony `lib` a bez přípony**:

- `-lgenerator` → linker hledá `libgenerator.so`, a když není, `libgenerator.a`
- `-lcompute` → linker hledá `libcompute.so`, a když není, `libcompute.a`

Proto se knihovny **musí** jmenovat `lib…….a` / `lib…….so`. Soubor pojmenovaný
`generator.a` linker prostě nenajde a bude tvrdit, že knihovna neexistuje.
Tohle je ta „chyba programátora“, o které mluví zadání.

Důležité je také **pořadí na příkazové řádce**: `-l` patří až **za** objektové
soubory, které knihovnu používají. Linker prochází argumenty zleva doprava
a z knihovny bere jen to, co mu v tu chvíli chybí.

```bash
g++ main1.o -L../static_lib -lgenerator -o main1
```

Tohle je správně. Opačné pořadí skončí na `undefined reference`:

```bash
g++ -L../static_lib -lgenerator main1.o -o main1
```

---

## 8. Jak dynamický linker hledá `.so` za běhu

Když spustíš program, jádro zavolá `ld.so` (dynamický linker), který podle
záznamů `NEEDED` doplní chybějící knihovny. Hledá v tomto pořadí:

1. `DT_RPATH` zapsaná v programu (kdyby se linkovalo s `-Wl,-rpath`) — my ji
   schválně nepoužíváme, právě aby šlo knihovny přepínat
2. **`LD_LIBRARY_PATH`** — proměnná prostředí, seznam adresářů oddělených `:`
3. `DT_RUNPATH` zapsaná v programu
4. cache `/etc/ld.so.cache`, kterou spravuje `ldconfig`
5. výchozí systémové adresáře `/lib`, `/usr/lib`, `/lib64`, `/usr/lib64`

Náš `libcompute.so` není nikde v systému, proto o něm `ld.so` neví — dokud mu
cestu neřekneme přes `LD_LIBRARY_PATH`. To je celý trik cvičení.

Celé hledání krok za krokem se dá sledovat (hodně výstupu, ale poučné):

```bash
LD_DEBUG=libs LD_LIBRARY_PATH=$PWD/dyn_lib1 ./main2/main2 < /dev/null
```

---

## 9. Chybové hlášky a co doopravdy znamenají

**`/usr/bin/ld: cannot find -lcompute`** — chyba **při linkování**. Linker nenašel
soubor knihovny. Zkontroluj v tomto pořadí: existuje soubor? jmenuje se
`libcompute.so`? sedí cesta u `-L`? Ověř `ls -l dyn_lib1/libcompute.so`.

**`fatal error: compute.h: No such file or directory`** — chyba **při překladu**.
Chybí nebo je špatně `-I`. Hlavička musí v tom adresáři opravdu být.

**`undefined reference to compute`** — linker knihovnu našel, ale funkci v ní ne.
Typicky chybí `-l` úplně, je ve špatném pořadí (kapitola 7), nebo se neshoduje
`extern "C"` mezi hlavičkou a zdrojákem knihovny.

**`error while loading shared libraries: libcompute.so: cannot open shared object file`**
— chyba **až při spuštění**, překlad proběhl v pořádku. Není nastavená
`LD_LIBRARY_PATH`, nebo je v ní chybná cesta, nebo se zapomnělo na `export`.

**`relocation R_X86_64_PC32 against symbol … can not be used when making a shared object; recompile with -fPIC`**
— do sdílené knihovny se dostal objektový soubor přeložený bez `-fPIC`.
Řešení: `make clean && make`.

**`Makefile:29: *** missing separator. Stop.`** — v Makefile je na začátku řádku
s příkazem mezera místo **tabulátoru**. Ověř `cat -A Makefile`, tabulátor se
zobrazí jako `^I`.

---

## 10. Diagnostické nástroje, které se hodí u obhajoby

```bash
file dyn_lib1/libcompute.so
```

```bash
ar t static_lib/libgenerator.a
```

```bash
nm -D --defined-only dyn_lib1/libcompute.so
```

```bash
nm -D --undefined-only main2/main2
```

```bash
ldd main2/main2
```

```bash
readelf -d main2/main2
```

Nejnázornější důkaz, že se opravdu mění knihovna a ne program:

```bash
LD_LIBRARY_PATH=$PWD/dyn_lib1 ldd main2/main2 | grep compute
```

```bash
LD_LIBRARY_PATH=$PWD/dyn_lib2 ldd main2/main2 | grep compute
```

Stejný program, dvě různé cesty ke knihovně.
