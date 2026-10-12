# Otázky k obhajobě — `exec` a `dup2`

Krátké odpovědi, tak jak se dají říct nahlas.

---

**Co dělá `dup2( a, b )`?**
Udělá z deskriptoru `b` kopii deskriptoru `a`. Když `b` bylo otevřené,
nejdřív ho zavře. `dup2( roura[1], 1 )` tedy znamená „stdout teď vede do
roury“.

**Proč po `dup2` zavíráš původní deskriptor?**
Po `dup2` vedou do roury dva deskriptory — původní a nový (0 nebo 1).
Potřebujeme jen ten nový. Kdyby původní zápisový konec zůstal otevřený,
čtenář by nikdy nedostal konec roury.

**Co dělá `exec`?**
Nahradí program běžícího procesu jiným programem. PID zůstává, paměť se
zahodí a načte se nový program. **Otevřené deskriptory zůstanou** — proto
`sort` po `dup2` čte ze souboru a píše do roury, aniž by o tom věděl.

**Proč je za `execlp` `perror` a `exit`?**
Když `exec` uspěje, nikdy se nevrátí — za ním už nic neběží. Kód za ním se
provede jen tehdy, když `exec` selže (např. program neexistuje).

**Jaký je rozdíl mezi `execlp` a `execvp`?**
`l` = argumenty se píšou jako seznam: `execlp( "nl", "nl", "-s", ". ", NULL )`.
`v` = argumenty jsou v poli: `execvp( "nl", pole )`. `p` = program se hledá
v proměnné `PATH`, takže stačí napsat `"sort"` místo `"/usr/bin/sort"`.

**Proč je `"nl"` v `execlp` dvakrát?**
První je jméno programu, který se má spustit. Druhé je `argv[0]` — jméno,
pod kterým program uvidí sám sebe. Seznam argumentů musí končit `NULL`.

**Proč se v `execlp` píše `". "` bez uvozovek navíc?**
Uvozovky v shellu jen drží mezeru u tečky, aby to byl jeden argument.
Program `nl` uvozovky nikdy nedostane — dostane jen dva znaky `. `.
V `execlp` je každý argument samostatný řetězec, žádné uvozovky navíc
nejsou potřeba.

**Proč se nesmí použít `system()` nebo `popen()`?**
Ty spustí shell, který udělá `fork`, `pipe`, `dup2` a `exec` za nás. Cílem
cvičení je udělat to sám.

**Proč rodič zavírá všechny roury?**
Rodič rouru nepoužívá. Kdyby nechal otevřený zápisový konec roury 1, `nl`
by po skončení `sort` čekal dál, `tr` taky a rodič by visel ve `wait()`.

**Jak tvůj `nl` poznává řádky, když z roury chodí data po kusech?**
Čte znak po znaku a pamatuje si jen, jestli je na začátku řádku. Na začátku
řádku rozhodne: když první znak je `\n`, řádek je prázdný a vypíše se jen
8 mezer, jinak se vypíše číslo. Nezáleží na tom, jak velké kusy z roury
přišly, a délka řádku není omezená.

**Proč `getchar` a ne `read` po jednom znaku?**
`read( 0, &c, 1 )` by bylo jedno systémové volání na každý znak — velmi
pomalé. `getchar` si načte celý blok do bufferu a znaky vydává z něj.

**Proč na konci druhého potomka `exit( 0 )` a ne `_exit( 0 )`?**
`printf` a `putchar` píšou do bufferu stdio. `exit()` ho před koncem
vyprázdní do roury, `_exit()` ne — poslední data by se ztratila.

**Co znamenají příznaky `O_WRONLY | O_CREAT | O_TRUNC` a `0644`?**
Jen pro zápis, vytvořit když neexistuje, zkrátit na 0 když existuje. To je
přesně chování `>` v shellu. `0644` jsou práva nového souboru `rw-r--r--`.

**Proč se výstup `diff out1 out2` může lišit na jiném počítači?**
Pořadí `sort` závisí na locale (`LANG`, `LC_ALL`). Program ale `sort`
spouští se stejným prostředím jako shell, takže na stejném počítači je
pořadí vždy stejné.

---

# Varianty (možná rozšíření)

**Jak přidáš do kolony další příkaz?**
Další rouru, další blok `if ( fork() == 0 )` se dvěma `dup2` a `execlp`,
a o jeden `wait` víc. Všichni (i rodič) musí zavřít i konce nové roury.

**Proč po `dup2` můžou potomci zavřít úplně všechny roury?**
Rouru, kterou potřebují, už mají zkopírovanou na deskriptoru 0 nebo 1.
Původní deskriptory z `pipe()` jsou zbytečné a jen by držely rouru otevřenou.

**Jak nahradíš příkaz vlastním kódem?**
Přesměrování zůstane stejné, jen místo `execlp` zavolám funkci, která čte
stdin a píše stdout, a pak `exit( 0 )`. Funkce neví, že čte z roury.

**Jak by vypadal vlastní `sort`?**
Načíst všechny řádky do dynamického pole (`getline`, `realloc`), seřadit
`qsort` a vypsat. Porovnávat `strcoll` (podle jazyka), protože tak řadí
program `sort`; bez `setlocale( LC_ALL, "" )` by `strcoll` řadil jako ASCII.

**Jak pošle jeden proces data do dvou rour?**
Každý přečtený blok zapíše `write()` do obou. To je vlastní `tee`.

**Jaký je rozdíl mezi `O_TRUNC` a `O_APPEND`?**
`O_TRUNC` soubor při otevření zkrátí na nulu (`>`), `O_APPEND` zapisuje vždy
na konec (`>>`).

**Jak přesměruješ chyby potomka?**
`dup2( fd, 2 )` — deskriptor 2 je stderr. Hlášky z `perror` pak jdou do
souboru.

**Jak se vytváří kolona v cyklu?**
Rodič si drží jen čtecí konec předchozí roury. V každém kroku vytvoří novou
rouru, potomek si vezme předchozí jako stdin a novou jako stdout. Rodič pak
zavře předchozí a zápisový konec nové; čtecí konec nové si nechá pro
dalšího potomka.

**Co znamená návratový kód 127?**
Konvence shellu pro „program nenalezen“. Potomek ho vrací, když `exec`
selže.

**Proč dostanou potomci `SIGPIPE`, když kolonu pošlu do `head`?**
`head` po pár řádcích skončí a zavře rouru. Kdo do ní pak zapisuje, dostane
`SIGPIPE`, skončí a zavře svou rouru — a tak dál až k prvnímu procesu.
