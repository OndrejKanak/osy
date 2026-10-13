# Otázky k obhajobě — procesy a roury

Krátké odpovědi, tak jak se dají říct nahlas.

---

**Co dělá `fork()`?**
Vytvoří kopii procesu. Oba procesy pokračují od stejného místa — potomek
dostane návratovou hodnotu `0`, rodič PID potomka, při chybě `-1`.

**Co potomek po `fork()` zdědí?**
Kopii paměti (proměnné, haldu) a **kopie všech otevřených deskriptorů**,
které ukazují na stejné roury a soubory jako u rodiče. Proto se roury
vytvářejí před `fork()`.

**Jak zajistíš, že všichni potomci mají stejného rodiče?**
Každý potomek svůj blok ukončí `exit( 0 )`. Nikdy tedy nedojde k dalšímu
`fork()` v kódu — ten provede jen rodič.

**Co vytvoří `pipe()`?**
Jednosměrný kanál v jádře a dva deskriptory: `[0]` pro čtení, `[1]` pro
zápis. Co se zapíše do `[1]`, přečte se z `[0]`.

**Proč za `fork()` každý zavírá, co nepotřebuje?**
Čtenář dostane konec roury (`read()` vrátí 0) teprve tehdy, když jsou
zavřené všechny zápisové konce ve všech procesech. Kdyby některý proces
zápisový konec zbytečně držel, čtenář by čekal navždy. A obráceně: kdyby
nikdo nedržel čtecí konec, zapisovatel dostane `SIGPIPE`.

**Co se stane, když rodič v 3. verzi zapomene zavřít `roura_a[1]`?**
Potomek 1 skončí, ale roura A má pořád otevřený zápisový konec u rodiče.
Potomek 2 tedy nedostane EOF a čeká dál, potomek 3 taky a rodič visí ve
`waitpid()`. Program se nikdy neukončí.

**Proč čteš rouru přes `fdopen()` a `fgets()`?**
Roura nemá hranice zpráv — `read()` může vrátit půl řádku nebo dva řádky
najednou. `fgets()` si data bufferuje a vrátí vždy právě jeden celý řádek.

**Proč zapisuješ přes `write()` a ne přes `fprintf()`?**
`write()` pošle data hned. `fprintf()` by je nechal ve stdio bufferu
a příjemce by je dostal až po naplnění bufferu nebo při ukončení.

**Proč je `fflush( stdout )` po každém výpisu?**
Při přesměrování do souboru nebo roury je stdout plně bufferovaný. Bez
`fflush` by se výpis objevil najednou až na konci.

**Proč `srand()` voláš až v potomkovi a s `getpid()`?**
Kdyby se generátor inicializoval jen časem, dva programy spuštěné ve stejné
sekundě by generovaly stejná data. `getpid()` je pro každý proces jiné.

**Jak generuješ náhodné datum?**
`rand() % 12` vybere měsíc, ten určí počet dní z pole
`{ 31, 28, 31, ... }`, a `rand() % dni + 1` vybere den. 29. 2. se tedy
nikdy nevygeneruje.

**Co dělá `waitpid()` a proč ho rodič volá?**
Počká na ukončení potomka a převezme jeho návratový kód. Bez toho by po
potomkovi zůstal *zombie* — záznam v tabulce procesů, dokud by rodič
neskončil.

**Jak poznáš, jestli potomek skončil normálně, nebo signálem?**
`WIFEXITED( stav )` → normální konec, kód je ve `WEXITSTATUS( stav )`.
`WIFSIGNALED( stav )` → zabit signálem, číslo je ve `WTERMSIG( stav )`.
Je to vidět při `./kalendar3 50 20 | head -2` — potomky ukončí `SIGPIPE` (13).

**Proč generátor používá `usleep()`?**
Zadání chce data posílat pomalu, aby bylo vidět, jak putují mezi procesy.
`usleep( 1000000 / N )` čeká mikrosekundy, takže `N` datumů za sekundu.

---

# Zadání — kalendar4

**Jak jsi zajistil, že kód 3. a 5. potomka je stejný?**
Oba volají funkci `vypis_s_cisly( fd, kdo )`. Liší se jen deskriptorem roury,
ze které čtou, a popiskem do výpisu. Funguje to, protože potomek 4 posílá
řádky ve stejném tvaru jako potomek 2: `den.mesic. Jmeno`.

**Kolik je tu rour a kolik konců musí každý proces zavřít?**
Čtyři roury = osm deskriptorů. Každý proces si nechá jen ty, které používá
(nejvýš dva), zbytek zavře funkcí `zavri_roury_krome()`. Rodič zavře všech osm.

**Co by se stalo, kdyby potomek 3 nezavřel `C[1]`?**
Roura C by měla otevřený zápisový konec i po skončení potomka 1. Potomek 4
by nikdy nedostal EOF, nezavřel by D, potomek 5 by čekal navždy a rodič by
visel ve `waitpid()`.

**Jak potomek 4 najde datum ke jménu?**
Projde seznam `g_svatky` a porovná jméno (`strcmp`) se druhým sloupcem.
Vrátí datum z prvního sloupce. Je to opak `najdi_svatek()`.

**Proč se výpisy P3 a P5 střídají?**
Jsou to dva nezávislé procesy, které píšou na stejný terminál. Pořadí určuje
plánovač jádra. Každý `printf` s `fflush` vypíše celý řádek najednou, takže
se řádky nerozbijí.

**Proč potomek 1 zapisuje do dvou rour?**
Zadání chce, aby posílal data potomkovi 2 a jména potomkovi 4. V jedné
smyčce vždy zapíše datum do A a jméno do C. Nakonec zavře obě roury.

---

# Varianty (možná rozšíření)

**Co dělá `dup2( a, b )`?**
Zavře deskriptor `b` a udělá z něj kopii `a`. `dup2( roura[0], 0 )` tedy
přesměruje stdin na rouru. Původní `roura[0]` se pak zavře, aby rouru
nedržely dva deskriptory.

**Co dělá `exec`?**
Nahradí program běžícího procesu jiným programem. PID zůstane, otevřené
deskriptory taky — proto `sort` po `dup2` čte z roury, aniž by o tom věděl.
Když `exec` uspěje, nikdy se nevrátí. Kód za ním běží jen při chybě.

**Proč se se `sort` vypíše všechno až na konci?**
`sort` musí mít všechny řádky, než může vypsat první. Čte tedy, dokud
nedostane konec roury.

**Co se stane, když dva procesy zapisují do jedné roury?**
Řádky se střídají, ale zápis do `PIPE_BUF` (4 kB na Linuxu) je atomický,
takže se nerozbijí uprostřed. Čtenář dostane EOF, až zavřou oba.

**Čím se liší pojmenovaná roura od `pipe()`?**
`pipe()` zdědí jen potomci přes `fork()`. Pojmenovaná roura (`mkfifo`) je
soubor v adresáři, otevřít ji může kterýkoli proces, který zná jméno.
Data se předávají v paměti stejně jako u `pipe()`.

**Proč `open()` na FIFO čeká?**
Otevření pro čtení čeká na zapisovatele a naopak. Teprve když jsou připojené
obě strany, `open()` se vrátí.

**K čemu je `poll()`?**
Čeká na více deskriptorů najednou a vrátí se, když je kterýkoli připravený.
Bez něj by se `read()` zablokoval na jedné rouře, i když ve druhé už data jsou.

**Proč se s `poll()` nepoužívá `fgets()`?**
stdio si načte data do svého bufferu. `poll()` o něm neví, hlásí jen data,
která jsou ještě v rouře. Celý řádek by mohl ležet v bufferu a `poll()` by
na něj zbytečně čekal.

**Komu pošle `Ctrl-C` signál?**
Terminál pošle `SIGINT` celé skupině procesů v popředí — rodiči i všem
potomkům najednou.

**Jak ukončíš kolonu procesů tak, aby se neztratila data?**
Potomci `SIGINT` ignorují. Rodič ho zachytí a pošle `SIGTERM` jen prvnímu
procesu (generátoru). Ten zavře rouru, další dostane EOF, zpracuje zbytek,
zavře svou rouru a tak dál. Kolona se vyprázdní zepředu dozadu.

**Co smí obsluha signálu dělat?**
Jen *async-signal-safe* funkce, například `write()` nebo `kill()`. Ne
`printf()` ani `malloc()`. Nejbezpečnější je jen nastavit příznak typu
`volatile sig_atomic_t`.

**Co je `SA_RESTART`?**
Když signál přeruší systémové volání (`read`, `waitpid`), jádro ho samo
zopakuje. Bez toho by vrátilo `-1` a `errno == EINTR`.
