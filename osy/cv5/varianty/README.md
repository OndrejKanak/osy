# Varianty — možná rozšíření přípravy `kolona`

Odhad, s čím může přijít skutečné zadání. Každá varianta je samostatný
program, všechny vycházejí z `kolona1.cpp` / `kolona2.cpp`.

```bash
cd ~/osy/cv5/varianty && make
```

`names.txt` dejte do stejné složky (nebo upravte cestu v příkazech).

| program | co přidává | klíčové funkce |
|---|---|---|
| `v1_ctyri_procesy` | čtvrtý potomek a třetí roura | `dup2`, `execlp` |
| `v2_vlastni` | příkazy nahrazené vlastním kódem | `getchar`, `getline`, `qsort` |
| `v3_rozvetveni` | jeden zdroj, dvě větve | vlastní `tee`, společná funkce |
| `v4_parametry` | stdin, `>>`, `2>`, oddělovač | `getopt`, `O_APPEND` |
| `v5_obecna_kolona` | libovolný počet příkazů | cyklus, `execvp` |
| `v6_stavy` | jak potomci skončili | `waitpid`, `WIFEXITED` |

Všechny jsou ověřené proti odpovídajícímu příkazu shellu na `names.txt`
(58 257 jmen) — `diff` je vždy prázdný.

**Společný trik všech variant:** po `dup2` má potomek rouru na deskriptoru
0 nebo 1, původní konce rour už nepotřebuje. Proto každý potomek (i rodič)
zavře **všechny** konce všech rour jednou funkcí `zavri_roury()`.

---

## V1 — čtvrtý proces

```
names.txt -> sort -r1-> nl -r2-> tr -r3-> head -n N -> stdout
```

```bash
sort < names.txt | nl -s ". " | tr a-z A-Z | head -n 10 > out1
./v1_ctyri_procesy names.txt 10 > out2
diff out1 out2
```

Co se změnilo proti `kolona1`: `POCET_ROUR 3`, čtvrtý blok `if ( fork() == 0 )`,
čtvrtý `wait`. Nic jiného.

**Co sledovat:** `head` skončí po 10 řádcích a zavře rouru 3. `tr` při dalším
zápisu dostane `SIGPIPE` a skončí, pak `nl` a `sort` stejně. Celá kolona se
ukončí sama.

---

## V2 — vlastní příkazy

```
names.txt -> [ sort (exec) ] -r1-> [ vlastní FILTR ] -r2-> [ vlastní tr ] -> stdout
```

```bash
./v2_vlastni names.txt nl           # sort | nl -s ". " | tr a-z A-Z
./v2_vlastni names.txt head 5       # sort | head -n 5  | tr a-z A-Z
./v2_vlastni names.txt grep an      # sort | grep -F an | tr a-z A-Z
./v2_vlastni names.txt wc           # sort | wc -l      | tr a-z A-Z
./v2_vlastni names.txt uniq         # sort | uniq       | tr a-z A-Z
./v2_vlastni names.txt sort         # sort | sort       | tr a-z A-Z
./v2_vlastni names.txt tr           # sort | tr a-z A-Z | tr a-z A-Z
```

Test každého filtru:

```bash
sort < names.txt | head -n 5 | tr a-z A-Z > out1
./v2_vlastni names.txt head 5 > out2
diff out1 out2
```

Vlastní implementace jsou v `vlastni.hpp`. Každá čte stdin a píše stdout,
takže se v potomkovi použije **místo `execlp`** po stejných `dup2`:

| funkce | nahrazuje | jak |
|---|---|---|
| `moje_nl()` | `nl -s ". "` | znak po znaku, stav „začátek řádku“ |
| `moje_tr()` | `tr a-z A-Z` | `c - 'a' + 'A'` jen pro `a`–`z` |
| `moje_head( n )` | `head -n N` | počítá `\n`, po N skončí |
| `moje_wc_l()` | `wc -l` | počítá `\n` |
| `moje_grep( t )` | `grep -F t` | `getline` + `strstr` |
| `moje_uniq()` | `uniq` | porovná řádek s předchozím |
| `moje_sort()` | `sort` | všechny řádky do pole, `qsort` + `strcoll` |

Každou funkci jsem kromě testu v koloně porovnal i napřímo se skutečným
programem na nesetříděných datech s duplicitami, prázdnými řádky,
diakritikou, řádkem dlouhým 100 000 znaků a posledním řádkem bez `\n`.

**Proč `moje_sort` používá `strcoll` a `setlocale`:** program `sort` neřadí
podle ASCII, ale podle jazyka (locale). V `en_US.UTF-8` je `anna` hned vedle
`Anna`, v ASCII by všechna velká písmena byla před malými. `strcoll` řadí
stejně jako `sort`; když vrátí 0, rozhodne `strcmp` — stejně jako GNU `sort`.

---

## V3 — rozvětvení

```
                               +-- roura A --> sort    --> vystup_a
names.txt --> [ rozdvojí ] ----+
                               +-- roura B --> sort -r --> vystup_b
```

```bash
./v3_rozvetveni names.txt a.txt b.txt
sort    < names.txt | diff - a.txt
sort -r < names.txt | diff - b.txt
```

- Potomek 1 čte soubor po blocích a **každý blok zapíše do obou rour** —
  vlastní `tee`.
- Potomek 2 a 3 dělají totéž, liší se jen směrem třídění a výstupním
  souborem → jedna funkce `setrid( roura, soubor, obracene )`.

---

## V4 — parametry a přesměrování

```bash
./v4_parametry names.txt                 # jako kolona1
./v4_parametry < names.txt               # vstup ze stdin (bez dup2)
cat names.txt | ./v4_parametry           # taky stdin
./v4_parametry names.txt out             # > out    (O_TRUNC)
./v4_parametry -a names.txt out          # >> out   (O_APPEND)
./v4_parametry -s ": " names.txt         # nl -s ": "
./v4_parametry -e chyby.txt neni.txt     # 2> chyby.txt
```

| v shellu | v programu |
|---|---|
| `< soubor` | `open( soubor, O_RDONLY )` + `dup2( fd, 0 )` |
| `> soubor` | `open( soubor, O_WRONLY \| O_CREAT \| O_TRUNC, 0644 )` + `dup2( fd, 1 )` |
| `>> soubor` | `open( soubor, O_WRONLY \| O_CREAT \| O_APPEND, 0644 )` + `dup2( fd, 1 )` |
| `2> soubor` | `open( ... )` + `dup2( fd, 2 )` |

Když vstup není zadaný, první potomek **nedělá nic** — stdin zdědí od
rodiče, a to je terminál, soubor z `<` nebo roura z `|`.

---

## V5 — obecná kolona

```bash
./v5_obecna_kolona names.txt sort , nl -s ". " , tr a-z A-Z
./v5_obecna_kolona names.txt cut -c1 , sort , uniq -c , sort -rn , head -3
```

Druhý příkaz vypíše tři nejčastější počáteční písmena jmen.

**Proč čárka:** `|` by si vzal shell. Argumenty rozdělí shell (i `". "`
zůstane jeden argument), program jen nahradí čárky za `NULL` a každý úsek
`argv` je hotové pole pro `execvp`.

**Cyklus:** rodič si drží jen čtecí konec předchozí roury.

```cpp
int predchozi = open( soubor, O_RDONLY );
for ( i = 0; i < pocet; i++ )
{
    if ( !posledni ) pipe( roura );
    if ( fork() == 0 )
    {
        dup2( predchozi, 0 );  close( predchozi );
        if ( !posledni ) { dup2( roura[1], 1 ); close( roura[0] ); close( roura[1] ); }
        execvp( prikaz[0], prikaz );
    }
    close( predchozi );                         // už ji má potomek
    if ( !posledni ) { close( roura[1] ); predchozi = roura[0]; }
}
```

---

## V6 — stavy potomků

```bash
./v6_stavy names.txt > /dev/null          # všichni s kódem 0
./v6_stavy neni.txt                       # sort s kódem 1
PATH=/nikde ./v6_stavy names.txt          # exec selže: kód 127
./v6_stavy names.txt | head -2            # SIGPIPE (13)
```

```
rodic: potomek 1 (sort) ukoncen signalem 13 (Broken pipe)
rodic: potomek 2 (nl) ukoncen signalem 13 (Broken pipe)
rodic: potomek 3 (tr) ukoncen signalem 13 (Broken pipe)
```

- `WIFEXITED( stav )` → skončil sám, kód je `WEXITSTATUS( stav )`
- `WIFSIGNALED( stav )` → zabil ho signál, číslo je `WTERMSIG( stav )`
- `127` je v shellu konvence pro „program nenalezen“
- rodič vrací 1, když kterýkoli potomek selhal (jako `set -o pipefail`)
