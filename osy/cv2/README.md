# Operační systémy — druhé cvičení: `myls`, `gennum`, `monitor`

Tři samostatné programy. Žádné knihovny, každý adresář má jeden `.cpp` a Makefile
převzatý přímo z učitelova dema.

---

## 1. Struktura

```
cv2/
├── Makefile              přeloží všechny tři programy
├── run_demo.sh           projede ukázkový scénář
├── README.md             tento soubor
├── OBHAJOBA.md           otázky k obhajobě i s odpověďmi
│
├── myls/myls.cpp         vlastní zjednodušené ls
├── gennum/gennum.cpp     generátor běžící až do Ctrl-C
└── monitor/monitor.cpp   vlastní obdoba tail -f
```

```bash
cd ~/osy/cv2 && chmod +x run_demo.sh && make
```

---

## 2. `myls` — vlastní `ls`

```bash
./myls/myls [-s] [-t] [-r] soubor ...
```

| přepínač | informace |
|---|---|
| `-s` | velikost souboru v bajtech |
| `-t` | čas poslední změny obsahu |
| `-r` | přístupová práva ve tvaru `-rw-r--r--` |

**Informace se vypisují v pořadí, v jakém jsou přepínače zadány.** Jméno souboru
je vždy poslední sloupec vpravo.

```bash
./myls/myls -s -t -r myls/myls.cpp
```

```
        6773 2026-09-28 20:46:41 -rw-r--r-- myls/myls.cpp
```

```bash
./myls/myls -r -t -s myls/myls.cpp
```

```
-rw-r--r-- 2026-09-28 20:46:41         6773 myls/myls.cpp
```

Testy ze zadání:

```bash
./myls/myls -s myls/myls.cpp
```

```bash
./myls/myls -r -s */*.cpp /etc/*.conf
```

```bash
./myls/myls -r -s -t *
```

Znak `*` rozvine shell, program tedy dostane rovnou seznam jmen — sám žádné
adresáře neprochází.

Soubory, které se nepodařilo najít, se vypíšou v souhrnu na konci a program
skončí s návratovým kódem 1:

```
        6773 myls/myls.cpp
        3821 gennum/gennum.cpp

Nenalezene soubory (2):
    tohle_neexistuje.txt
    ani_tohle.c
```

### Proč `-t` ukazuje čas změny obsahu

Zadání mluví o „času vytvoření", jenže **Linux ho ve `struct stat` vůbec nemá**.
K dispozici jsou jen tři časy:

| položka | co znamená |
|---|---|
| `st_atime` | poslední **přístup** (čtení) |
| `st_mtime` | poslední změna **obsahu** — tohle ukazuje `ls -l` |
| `st_ctime` | poslední změna **i-uzlu** (práva, vlastník, počet odkazů) |

`st_ctime` se často mylně čte jako „creation time", ale `c` je od *change*.
Skutečný čas vytvoření (*birth time*) existuje až v novějším rozhraní `statx()`
a ne na každém souborovém systému. Program proto vypisuje `st_mtime` — tedy
to, co uživatel od `ls -l` čeká. Kdyby učitel chtěl `st_ctime`, je to změna
jednoho slova v `myls.cpp`.

### Dvě věci, které stojí za zmínku

Přepínače čte `getopt()`, která je vrací **přesně v pořadí, v jakém stojí na
příkazové řádce** — stačí si je tedy postupně ukládat do pole a pak podle něj
vypisovat sloupce. Funguje i sloučený zápis `-rst`.

Informace o souboru se zjišťují funkcí `lstat()`, ne `stat()`. Rozdíl je
u symbolických odkazů: `lstat()` popíše odkaz samotný (proto se u něj objeví
typ `l`), `stat()` by šel po odkazu na cíl. `ls -l` se chová stejně jako
`lstat()`.

---

## 3. `gennum` — generátor běžící až do `Ctrl-C`

```bash
./gennum/gennum M N > out.txt
```

- `M` = **maximální** počet čísel na řádku (skutečný počet je náhodný 1 až M)
- `N` = počet řádků za minutu

```bash
./gennum/gennum 10 60 > out.txt
```

Tohle vypíše zhruba jeden řádek za sekundu, dokud program neukončíš `Ctrl-C`.
Čísla jsou z rozsahu 10 až 1000, tedy nikdy ne jednociferná.

### ⚠️ Past, na které cvičení stojí nebo padá

Když výstup nejde na terminál, ale **do souboru**, je `stdout` plně bufferovaný
(zhruba 4 kB). Bez explicitního `fflush( stdout )` po každém řádku by se do
souboru dlouhé minuty nic nezapsalo a `tail -f` ani `monitor` by neměly co
ukazovat. Na terminálu se problém neprojeví, protože tam je `stdout`
bufferovaný po řádcích — o to zákeřnější to je.

Ověřit se to dá snadno: spusť generátor, počkej pár vteřin a zkontroluj
velikost souboru.

```bash
./gennum/gennum 10 120 > out.txt &
sleep 3
wc -c out.txt
```

Nenulová velikost znamená, že `fflush()` dělá svou práci.

### Časování bez driftu

Čas dalšího řádku se počítá **absolutně** od startu programu, ne jako „po
každém řádku počkej interval". Kdyby se čekal jen interval, přičítala by se
k němu pokaždé doba samotného výpisu a generátor by se postupně zpožďoval.
Slouží k tomu `clock_nanosleep()` s příznakem `TIMER_ABSTIME`.

`Ctrl-C` obsluhuje vlastní handler, takže se program ukončí čistě — dopíše
rozdělaný řádek a na `stderr` vypíše, kolik řádků stihl vygenerovat.

---

## 4. `monitor` — vlastní `tail -f`

```bash
./monitor/monitor [-a] soubor
```

- bez přepínače přeskočí stávající obsah a vypisuje jen to, co nově přibude
  (přesně jako `tail -f`)
- `-a` vypíše nejdřív celý stávající obsah a teprve pak sleduje přírůstky

Obsah souboru jde na `stdout`, hlášení o změnách velikosti na `stderr`. Díky
tomu se dá obsah přesměrovat a hlášení přitom zůstanou vidět na terminálu:

```bash
./monitor/monitor out.txt > kopie.txt
```

Ukázka hlášení:

```
[2026-09-28 20:54:38] sleduji soubor out.txt, velikost 16 B
Ukonceni pomoci Ctrl-C.
[2026-09-28 20:54:41] velikost: 16 -> 32 B (+16)
[2026-09-28 20:54:43] soubor zkracen: 32 -> 0 B (-32)
[2026-09-28 20:54:43] ctu znovu od zacatku souboru.
[2026-09-28 20:54:45] velikost: 0 -> 14 B (+14)
```

### Jak to uvnitř funguje

Soubor se otevře **jednou** pomocí `open()` a po celou dobu běhu se nezavírá.
Jednou za sekundu se zavolá `fstat()` **nad otevřeným deskriptorem** (ne `stat()`
nad jménem) a podle zjištěné velikosti se:

- doctou nové bajty pomocí `read()`, když soubor narostl
- skočí `lseek()` zpátky na začátek, když je soubor kratší než přečtená pozice

Když se soubor nedá číst, program končí — ať už selže `open()`, `fstat()`
nebo `read()`.

---

## 5. Scénář ze zadání krok za krokem

Potřebuješ tři terminály (nebo `screen`/`tmux`).

**Terminál 1 — generátor:**

```bash
cd ~/osy/cv2 && ./gennum/gennum 10 60 > out.txt
```

**Terminál 2 — nejdřív referenční `tail -f`, pak vlastní monitor:**

```bash
cd ~/osy/cv2 && tail -f out.txt
```

```bash
cd ~/osy/cv2 && ./monitor/monitor out.txt
```

Oba mají ukazovat totéž.

**Terminál 3 — zkrácení souboru:**

```bash
truncate -s 0 ~/osy/cv2/out.txt
```

### Proč generátor před zkrácením zastavit

Zadání to říká výslovně a má to dobrý důvod. Když generátor běží dál, má
v otevřeném souboru **vlastní pozici zápisu**. Po `truncate -s 0` píše pořád na
původní pozici, soubor se okamžitě zvětší zpátky (mezera se vyplní nulami) a
monitor, který kontroluje jen jednou za sekundu, zkrácení vůbec nemusí zahlédnout.

Správné pořadí je tedy: zastavit generátor (`Ctrl-C`) → `truncate -s 0` →
spustit generátor znovu.

---

## 6. Časté potíže

**`tail -f` ani `monitor` nic neukazují, i když generátor běží**
Chybí `fflush( stdout )` po každém řádku. Viz kapitola 3.

**Monitor nezahlédl zkrácení souboru**
Běžel při tom generátor. Viz kapitola 5.

**`truncate: command not found`**
Na některých systémech není `truncate`. Stejný efekt má přesměrování prázdného
výstupu:

```bash
: > out.txt
```

**`myls` u symlinku ukazuje divnou velikost**
To je správně — `lstat()` u symbolického odkazu vrací délku cílové cesty
v bajtech, ne velikost cílového souboru.

**Program po `Ctrl-C` nekončí hned**
Handler nastaví jen příznak; program doběhne rozdělaný krok. U monitoru to může
trvat až do konce probíhajícího `sleep( 1 )`.

---

## 7. Co si u obhajoby pustit

```bash
strace -e trace=openat,read,lseek,fstat ./monitor/monitor out.txt
```

Ukáže přesně ta systémová volání, o kterých zadání mluví — že se soubor otevře
jednou a pak se jen dokola střídá `fstat`, `read` a případně `lseek`.

```bash
ls -l /proc/$( pgrep -f 'monitor out.txt' )/fd
```

Ukáže otevřený deskriptor monitoru. Když soubor mezitím smažeš, objeví se
u něj `(deleted)` — a monitor přesto čte dál, protože i-uzel drží naživu
právě ten otevřený deskriptor.

Podrobnosti a další otázky jsou v [OBHAJOBA.md](OBHAJOBA.md).
