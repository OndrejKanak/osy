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
