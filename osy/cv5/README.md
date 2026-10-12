# Operační systémy — příprava: `exec` a `dup2` (`kolona`)

Program napodobuje složený příkaz shellu:

```bash
sort < names.txt | nl -s ". " | tr a-z A-Z
```

Seznam jmen se setřídí, řádky se očíslují a malá písmena se převedou na velká.

```
cv5/
├── Makefile
├── README.md
├── OBHAJOBA.md
└── kolona/
    ├── Makefile
    ├── kolona1.cpp   1. krok: 3 potomci, všichni přes exec
    ├── kolona2.cpp   2. krok: druhý potomek čísluje řádky sám
    └── kolona3.cpp   3. krok: volitelný výstupní soubor
```

```bash
cd ~/osy-git/osy/cv5 && make
```

Soubor `names.txt` si stáhněte z Kelvinu a dejte ho do složky `cv5`.

---

## Schéma

```
                 +-------+
                 | RODIČ |   vytvoří roury a potomky, zavře roury, čeká
                 +---+---+
         +-----------+-----------+
         v           v           v
   +-----------+ +-----------+ +-----------+
   | POTOMEK 1 | | POTOMEK 2 | | POTOMEK 3 |
   |   sort    | |    nl     | |    tr     |
   +-----------+ +-----------+ +-----------+

 names.txt --> sort --roura 1--> nl -s ". " --roura 2--> tr a-z A-Z --> stdout
```

| proces | stdin (0) | stdout (1) | spustí |
|---|---|---|---|
| potomek 1 | soubor (`open` + `dup2`) | roura 1 | `sort` |
| potomek 2 | roura 1 | roura 2 | `nl -s ". "` |
| potomek 3 | roura 2 | obrazovka (nebo soubor v kroku 3) | `tr a-z A-Z` |

---

## Krok 1 — `kolona1`: všichni tři přes `exec`

Každý potomek udělá tři věci:

1. **`dup2`** — přesměruje stdin/stdout do souboru nebo do roury.
2. **`close`** — zavře všechny konce rour (už má kopie na 0 a 1).
3. **`execlp`** — změní se v zadaný program.

```cpp
if ( fork() == 0 )
{
    int x = open( argv[ 1 ], O_RDONLY );
    dup2( x, 0 );               // stdin  = soubor
    dup2( roura1[ 1 ], 1 );     // stdout = roura 1
    close( x );
    close( roura1[ 0 ] ); close( roura1[ 1 ] );
    close( roura2[ 0 ] ); close( roura2[ 1 ] );
    execlp( "sort", "sort", NULL );
    perror( "sort" );           // sem se dojde jen když exec selže
    exit( 1 );
}
```

`sort` pak čte „svůj stdin“ a píše na „svůj stdout“. Vůbec neví, že jde
o soubor a rouru. Přesně tohle dělá shell, když napíšete `<` a `|`.

**Test:**

```bash
sort < names.txt | nl -s ". " | tr a-z A-Z > out1
./kolona/kolona1 names.txt > out2
diff out1 out2
```

`diff` nesmí nic vypsat.

---

## Krok 2 — `kolona2`: vlastní `nl`

Druhý potomek má `execlp( "nl", ... )` zakomentované a místo něj volá funkci
`cisluj_radky()`. Přesměrování (`dup2`) zůstává stejné — funkce čte stdin
a píše stdout, jako by to byl program `nl`.

**Co přesně dělá `nl -s ". "`** (ověřeno proti GNU `nl`):

| vstup | výstup |
|---|---|
| `anna` | `     1. anna` — číslo na 6 míst, pak `". "` |
| prázdný řádek | 8 mezer, **nečísluje se** |
| řádek jen z mezer | čísluje se (není prázdný) |
| poslední řádek bez `\n` | `\n` se doplní |

**Proč „správné zpracování řádků“:** z roury nechodí data po řádcích, ale po
libovolných kusech. Jeden `read()` může vrátit půl řádku nebo deset řádků
najednou. Proto se čte **znak po znaku** (`getchar`, které si data samo
bufferuje) a pamatuje se jen jedna věc: jestli jsme na začátku řádku. Délka
řádku tak není nijak omezená — žádný buffer `char radek[100]`, který by se
mohl přeplnit.

```cpp
while ( ( c = getchar() ) != EOF )
{
    if ( zacatek_radku )
    {
        if ( c == '\n' ) printf( "%6s  ", "" );        // prázdný: 8 mezer
        else             printf( "%6d. ", ++cislo );   // číslo + ". "
    }
    putchar( c );
    zacatek_radku = ( c == '\n' );
}
if ( !zacatek_radku ) putchar( '\n' );                  // chybějící '\n'
```

**Test:**

```bash
./kolona/kolona2 names.txt > out2
diff out1 out2
```

---

## Krok 3 — `kolona3`: výstup do souboru

Když se zadá druhý parametr, třetí potomek si před `exec` přesměruje stdout:

```cpp
if ( argc == 3 )
{
    int y = open( argv[ 2 ], O_WRONLY | O_CREAT | O_TRUNC, 0644 );
    dup2( y, 1 );               // stdout = soubor
    close( y );
}
execlp( "tr", "tr", "a-z", "A-Z", NULL );
```

- `O_CREAT` — soubor se vytvoří, když neexistuje
- `O_TRUNC` — když existuje, zkrátí se na 0 (jako `>` v shellu)
- `0644` — práva nového souboru `rw-r--r--`

**Test:**

```bash
./kolona/kolona3 names.txt out2
diff out1 out2
```

---

## Ověřené testy

Každý program jsem porovnal s příkazem shellu na třech souborech:

| soubor | obsah | kolona1 | kolona2 | kolona3 | kolona3 → soubor |
|---|---|---|---|---|---|
| testovací | 5000 jmen, prázdné řádky, řádek z mezer, řádek 100 000 znaků, diakritika, poslední řádek bez `\n` | ✔ | ✔ | ✔ | ✔ |
| malý | 2 jména | ✔ | ✔ | ✔ | ✔ |
| prázdný | nic | ✔ | ✔ | ✔ | ✔ |

Funkci `cisluj_radky()` jsem navíc porovnal přímo s `nl -s ". "` na
nesetříděném souboru (kde poslední řádek opravdu nemá `\n`) — výstup je
bajt po bajtu shodný.

## Omezení

GNU `nl` zná ještě speciální řádky `\:`, `\:\:`, `\:\:\:` (oddělovače
stránek). Vlastní implementace je neřeší — v seznamu jmen se nevyskytují.
