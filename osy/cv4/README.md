# Operační systémy — příprava: procesy a roury (`kalendar`)

Tři postupné verze jednoho programu. Každá je samostatný zdroják, všechny
sdílejí seznam jmenin v `svatky.hpp`.

```
cv4/
├── Makefile
├── README.md
├── OBHAJOBA.md
├── kalendar/
│   ├── Makefile
│   ├── svatky.hpp      jmeniny + pole dní v měsíci + najdi_svatek()
│   ├── kalendar1.cpp   1 potomek,  1 roura
│   ├── kalendar2.cpp   2 potomci,  1 roura
│   └── kalendar3.cpp   3 potomci,  2 roury
└── varianty/           možná rozšíření pro skutečné zadání
    ├── README.md       popis a testování všech variant
    ├── v1_exec.cpp             dup2 + exec (potomek = sort)
    ├── v2_dva_generatory.cpp   dva zapisovatelé, jedna roura
    ├── v3_fifo_gen.cpp         pojmenovaná roura - zapisovatel
    ├── v3_fifo_cti.cpp         pojmenovaná roura - čtenář
    ├── v4_poll.cpp             čtení ze dvou rour přes poll()
    └── v5_signaly.cpp          Ctrl-C a SIGUSR1
```

```bash
cd ~/osy-git/osy/cv4 && make
```

Všechny verze mají stejné parametry:

```bash
./kalendar/kalendarX M N
```

- `M` = kolik datumů se vygeneruje
- `N` = kolik datumů za sekundu (mezi datumy `usleep( 1000000 / N )`)

---

## Verze 1 — jeden potomek

```
  +---------+   roura    +---------+
  | POTOMEK | ---------> |  RODIČ  | ---> stdout
  +---------+  "d.m\n"   +---------+
   generuje               doplní jméno
```

```bash
./kalendar/kalendar1 5 2
```

```
24.10. Nina
24.7. Kristyna
8.1. Cestmir
8.4. Ema
18.6. Milan
rodic: generator 466 skoncil se stavem 0
```

## Verze 2 — dva potomci

Kód rodiče z verze 1 se přesunul do druhého potomka. Rodič jen vytvoří
potomky, zavře rouru a čeká.

```bash
./kalendar/kalendar2 5 2
```

## Verze 3 — tři potomci, dvě roury

```
          +-----------------------+
          | HLAVNI RODIC (Parent) |
          +-----------+-----------+
                      |
        +-------------+-------------+
        v             v             v
  +-----------+ +-----------+ +-----------+
  | POTOMEK 1 | | POTOMEK 2 | | POTOMEK 3 |
  +---+-------+ +---+---+---+ +-------+---+
      |   ROURA A   ^   |   ROURA B   ^
      +-------------+   +-------------+
```

| proces | čte | zapisuje | dělá |
|---|---|---|---|
| potomek 1 | — | roura A | generuje `den.mesic\n` |
| potomek 2 | roura A | roura B | doplní jméno `den.mesic. Jmeno\n` |
| potomek 3 | roura B | stdout | přidá číslo řádku `(1) den.mesic. Jmeno` |
| rodič | — | — | vytvoří roury a potomky, zavře roury, čeká |

```bash
./kalendar/kalendar3 6 2
```

```
(1) 7.11. Saskie
(2) 15.9. Jolana
(3) 16.6. Zbynek
(4) 5.4. Miroslava
(5) 29.11. Zina
(6) 30.9. Jeronym
rodic: potomek 1 (511) skoncil se stavem 0
rodic: potomek 2 (512) skoncil se stavem 0
rodic: potomek 3 (513) skoncil se stavem 0
```

---

## Struktura kódu: žádné vnořené `if`

Všechny `fork()` jsou na stejné úrovni odsazení, každý potomek končí `exit()`,
takže za jeho blok se nikdy nedostane:

```cpp
pid_t p1 = fork();
if ( p1 == 0 ) { ... kód potomka 1 ...; exit( 0 ); }

pid_t p2 = fork();
if ( p2 == 0 ) { ... kód potomka 2 ...; exit( 0 ); }

pid_t p3 = fork();
if ( p3 == 0 ) { ... kód potomka 3 ...; exit( 0 ); }

... kód rodiče: zavřít roury, waitpid() ...
```

Díky `exit( 0 )` jsou všichni potomci dětmi **jednoho** rodiče — potomek 1
nikdy nedojde k druhému `fork()`.

## Za `fork` každý zavírá, co nepotřebuje

Roury se vytvoří **před** forky, takže je zdědí všichni. Každý proces pak
hned zavře konce, které nepoužívá:

| proces | `A[0]` čtení | `A[1]` zápis | `B[0]` čtení | `B[1]` zápis |
|---|---|---|---|---|
| potomek 1 | zavře | **používá** | zavře | zavře |
| potomek 2 | **používá** | zavře | zavře | **používá** |
| potomek 3 | zavře | zavře | **používá** | zavře |
| rodič | zavře | zavře | zavře | zavře |

Proč je to důležité: čtenář dostane konec roury (EOF, `fgets()` vrátí
`NULL`) až ve chvíli, kdy jsou zavřené **všechny** zápisové konce v **celém
systému**. Kdyby si třeba rodič nechal otevřený `A[1]`, potomek 2 by po
skončení generátoru čekal navždy a program by se nikdy neukončil.

---

## Testování

**Pomalé předávání dat** — je vidět, jak data přicházejí po jednom:

```bash
./kalendar/kalendar3 10 1
```

**Strom procesů** — v druhém terminálu během běhu:

```bash
pstree -p $(pgrep -o kalendar3)
```

```
kalendar3(1000)─┬─kalendar3(1001)
                ├─kalendar3(1002)
                └─kalendar3(1003)
```

Všichni tři potomci visí přímo pod jedním rodičem.

**Otevřené deskriptory** — ověření, že každý zavřel, co nepotřebuje:

```bash
./kalendar/kalendar3 30 2 > /dev/null &
sleep 1
for p in $(pgrep kalendar3); do echo "== $p"; ls -l /proc/$p/fd | grep pipe; done
wait
```

- rodič: žádná roura
- potomek 1: jedna roura (zápis do A)
- potomek 2: dvě roury (čtení z A, zápis do B)
- potomek 3: jedna roura (čtení z B)

**Předčasně ukončený čtenář** — `head` po dvou řádcích skončí:

```bash
./kalendar/kalendar3 50 20 | head -2
```

```
(1) 4.3. Stela
(2) 14.1. Radovan
rodic: potomek 1 (520) ukoncen signalem 13 (Broken pipe)
rodic: potomek 2 (521) ukoncen signalem 13 (Broken pipe)
rodic: potomek 3 (522) ukoncen signalem 13 (Broken pipe)
```

Potomek 3 zapíše do roury, kterou už nikdo nečte, a dostane `SIGPIPE`.
Tím se zavře roura B, potomek 2 při dalším zápisu dostane `SIGPIPE`, a stejně
tak potomek 1. Řetěz se rozpadne odzadu.

**Žádné neznámé datum**:

```bash
./kalendar/kalendar3 3000 100000 2>/dev/null | grep -c '???'
```

Vypíše `0` — každé vygenerované datum má v seznamu jméno.
