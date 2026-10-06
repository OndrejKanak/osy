# Varianty — možná rozšíření přípravy `kalendar`

Odhad, s čím může přijít skutečné zadání na cvičení. Každá varianta je
samostatný program, všechny sdílejí `spolecne.hpp` a seznam svátků
z `../kalendar/svatky.hpp`.

```bash
cd ~/osy-git/osy/cv4/varianty && make
```

| program | téma | nové funkce |
|---|---|---|
| `v1_exec` | potomek spustí externí program | `dup2()`, `execvp()` |
| `v2_dva_generatory` | dva zapisovatelé do jedné roury | atomický zápis |
| `v3_fifo_gen` + `v3_fifo_cti` | pojmenovaná roura, dva terminály | `mkfifo()`, `open()` |
| `v4_poll` | jeden čtenář, dvě roury | `poll()` |
| `v5_signaly` | korektní `Ctrl-C`, hlášení rodiči | `sigaction()`, `kill()` |

---

## V1 — `dup2` + `exec`: potomek 3 je externí program

```
P1 generuje ──A──> P2 doplní jméno ──B──> P3 = sort (nebo jiný program) ──> stdout
```

```bash
./v1_exec 10 5                    # výchozí: sort podle měsíce a dne
./v1_exec 10 5 cat -n             # očíslované řádky
./v1_exec 500 1000 wc -l          # vypíše 500
./v1_exec 50 100 grep Jan         # jen řádky s "Jan"
./v1_exec 3 5 neexistuje          # exec selže -> stav 127
```

**Co sledovat:** se `sort` se 10 datumů generuje 2 sekundy a **teprve pak**
se vypíše všechno najednou. `sort` musí znát všechny řádky, než může první
vypsat — čte, dokud nedostane konec roury. S `cat -n` řádky přibývají
průběžně.

**Jak to funguje:**

```cpp
dup2( roura_b[ 0 ], STDIN_FILENO );   // stdin = čtecí konec roury B
close( roura_b[ 0 ] );                // původní deskriptor už není potřeba
execvp( "sort", argumenty );          // program se nahradí programem sort
perror( "sort" );                     // sem se dojde JEN když exec selže
exit( 127 );
```

`exec` nahradí kód procesu jiným programem, ale **otevřené deskriptory
zůstanou**. `sort` tedy čte „svůj stdin“ a neví, že je to roura. Přesně
tak funguje `|` v shellu.

---

## V2 — dva generátory do jedné roury

```
G1 ──┐
     ├──A──> příjemce ──> stdout
G2 ──┘
```

```bash
./v2_dva_generatory 5 4 1         # G1 je 4x rychlejší než G2
```

```
[generator 1] 4.8. Dominik
[generator 2] 11.7. Olga
[generator 1] 1.4. Hugo
...
prijemce: od generatoru 1 prislo 5, od generatoru 2 5 datumu
```

**Co sledovat:**
- Řádky obou generátorů se střídají, ale **nikdy se nerozbijí uprostřed**.
  Zápis do roury do 4 kB (`PIPE_BUF`) je atomický — proto se každý řádek
  posílá jedním `write()`.
- Příjemce skončí až po **pomalejším** generátoru. Konec roury přijde, až
  jsou zavřené zápisové konce u obou.

**Zátěžový test** — 6000 řádků, žádný rozbitý:

```bash
./v2_dva_generatory 3000 100000 100000 2>&1 >/dev/null | grep prijemce
```

---

## V3 — pojmenovaná roura (FIFO), dva terminály

Obyčejnou rouru (`pipe`) znají jen procesy, které ji zdědí přes `fork`.
Pojmenovaná roura je **soubor v adresáři** — otevřít ji může kdokoli, kdo
zná jméno. Data na disk nejdou, jádro je předává v paměti.

**Terminál 1:**
```bash
cd ~/osy-git/osy/cv4/varianty
./v3_fifo_cti kalendar.fifo
```
Vypíše `cekam na generator...` a **zablokuje se** v `open()`.

**Terminál 2:**
```bash
cd ~/osy-git/osy/cv4/varianty
./v3_fifo_gen kalendar.fifo 10 2
```
V tu chvíli se oba odblokují a data začnou téct.

**Co sledovat:**
- `open()` na FIFO čeká, dokud se nepřipojí druhá strana. Je jedno, kdo
  začne první.
- `ls -l kalendar.fifo` ukáže typ `p` (pipe): `prw-------`.
- Čtenář skončí, až se odpojí všichni zapisovatelé.
- Místo programů jdou použít obyčejné příkazy:
  ```bash
  cat kalendar.fifo                  # místo čtenáře
  echo 24.12 > kalendar.fifo         # místo generátoru
  ```
- Dva generátory najednou (terminál 2 a 3) — čtenář dostane data od obou.
- Na konci: `rm kalendar.fifo`

---

## V4 — `poll()`: čtení ze dvou rour najednou

```
G1 ──A──┐
        ├──> příjemce (poll) ──> stdout
G2 ──B──┘
```

```bash
./v4_poll 5 4 1
```

```
[A] 1.2. Hynek
[B] 23.2. Svatopluk
[A] 16.5. Premysl
[A] 28.9. Vaclav
[A] 30.5. Ferdinand
[B] 5.12. Jitka
prijemce: roura A skoncila
[B] 20.6. Kveta
...
prijemce: roura B skoncila
```

**Proč `poll()`:** kdyby příjemce četl `read()` nejdřív z A, zablokoval by
se tam — i když v B už čekají data. `poll()` čeká na **více deskriptorů
najednou** a vrátí se, když je kterýkoli připravený.

**Proč tu není `fgets()`:** stdio si načítá data do svého bufferu „do
zásoby“. `poll()` o tom bufferu neví a hlásí jen to, co je ještě v rouře.
Řádek by tak mohl ležet v bufferu a `poll()` by čekal zbytečně. Proto se
čte přímo `read()` a řádky se skládají ručně.

**Konec roury:** `read()` vrátí 0 → rouru zavřu a nastavím jí `fd = -1`,
`poll()` záporné deskriptory přeskakuje.

---

## V5 — signály

Zapojení jako `kalendar3`, navíc:
- `M = 0` generuje do nekonečna, ukončí se `Ctrl-C`
- `Ctrl-C` ukončí program **korektně**, žádné datum se neztratí
- potomek 3 posílá rodiči `SIGUSR1` po každých 5 řádcích

```bash
./v5_signaly 0 5
# po chvíli Ctrl-C
```

```
rodic: PID 673  (zkus v jinem terminalu: kill -USR1 673)
(1) 9.9. Daniela
...
(5) 20.5. Zbysek
rodic: SIGUSR1 - potomek 3 hlasi dalsich 5 radku
...
^C
rodic: Ctrl-C - posilam SIGTERM generatoru
potomek 1: dostal SIGTERM, odeslano 14 datumu
potomek 3: konec roury B, vypsano 14 radku
rodic: vsichni potomci skoncili, prijato 2 hlaseni SIGUSR1
```

**Odesláno 14 = vypsáno 14** — nic se neztratilo.

**Jak to funguje:**
1. `Ctrl-C` pošle terminál `SIGINT` **celé skupině procesů** — rodiči
   i všem potomkům.
2. Potomci `SIGINT` ignorují (rodič nastavil `SIG_IGN` před `fork`,
   potomci to zdědili).
3. Rodič `SIGINT` zachytí a pošle `SIGTERM` jen generátoru.
4. Generátor dopíše rozpracovaný řádek, zavře rouru A a skončí.
5. Potomek 2 dostane konec roury A, zpracuje zbytek, zavře B.
6. Potomek 3 dostane konec roury B, vypíše zbytek a skončí.

Kdyby potomci `SIGINT` neignorovali, umřeli by všichni najednou a data
rozpracovaná v rourách by se ztratila.

**Ruční posílání signálů** z druhého terminálu:

```bash
kill -USR1 <PID rodiče>     # rodič vypíše hlášku
kill -INT  <PID rodiče>     # totéž co Ctrl-C
```

**V obsluze signálu** se smí volat jen *async-signal-safe* funkce —
`write()`, `kill()` ano, `printf()` ne. Obsluha proto buď jen nastaví
příznak (`volatile sig_atomic_t`), nebo použije `write()`.

**`SA_RESTART`:** když signál přeruší systémové volání (`read`, `waitpid`),
jádro ho samo zopakuje. Bez toho by `waitpid()` vrátil `-1` s `EINTR`.
