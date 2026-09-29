# Otázky k obhajobě — druhé cvičení

Krátké odpovědi, tak jak se dají říct nahlas.

---

## myls

**Jak zajistíš, že se informace vypisují v pořadí zadaných přepínačů?**
`getopt()` vrací přepínače přesně v tom pořadí, v jakém stojí na příkazové
řádce. Stačí si je postupně ukládat do pole `options[]` a při výpisu každého
souboru projít to pole a vypsat sloupce v uloženém pořadí.

**Co je `optind`?**
Index prvního argumentu, který už není přepínač. Od něj až do `argc` jsou
jména souborů.

**Jaký je rozdíl mezi `stat()` a `lstat()`?**
`stat()` následuje symbolické odkazy a popíše cíl, `lstat()` popíše odkaz
samotný. Používám `lstat()`, protože stejně se chová `ls -l` — jinak by se
symlink nikdy neukázal jako typ `l`.

**A `fstat()`?**
Totéž, ale místo jména bere otevřený file deskriptor. Používám ho v `monitor`,
kde je to podstatné — deskriptor ukazuje pořád na stejný i-uzel, i kdyby se
soubor mezitím přejmenoval nebo smazal.

**Jak z `st_mode` dostaneš práva?**
Maskováním jednotlivých bitů: `st_mode & S_IRUSR` je právo čtení pro
vlastníka, `S_IWGRP` zápis pro skupinu a tak dál. Typ souboru se testuje
makry `S_ISREG`, `S_ISDIR`, `S_ISLNK` a podobně.

**Co znamená `s` nebo `t` místo `x` ve výpisu práv?**
Zvláštní bity. `s` na místě `x` u vlastníka je setuid (program běží s právy
vlastníka souboru), u skupiny setgid. `t` u ostatních je sticky bit — na
adresáři znamená, že soubor v něm smí smazat jen jeho vlastník, typicky `/tmp`.
Velké `S` nebo `T` znamená, že je zvláštní bit nastaven, ale `x` není.

**Proč `-t` ukazuje čas změny obsahu, když zadání říká čas vytvoření?**
Protože Linux čas vytvoření ve `struct stat` nemá. Jsou tam jen `st_atime`
(přístup), `st_mtime` (změna obsahu) a `st_ctime` (změna i-uzlu). Vypisuju
`st_mtime`, což je totéž, co ukazuje `ls -l`. Skutečný *birth time* existuje
až přes `statx()` a ne na každém souborovém systému.

**Proč se `st_ctime` nedá brát jako čas vytvoření?**
`c` je od *change*, ne *create*. Mění se při každé změně i-uzlu — když se
změní práva, vlastník nebo počet odkazů. U nově vytvořeného souboru jsou
`ctime` a čas vzniku shodou okolností stejné, ale při prvním `chmod` se to
rozejde.

**Jak vypisuješ čas?**
`localtime()` převede sekundy od 1. 1. 1970 na místní čas rozložený do položek,
`strftime()` ho pak naformátuje na `%Y-%m-%d %H:%M:%S`. Obě patří do skupiny
popsané v `man asctime`. Šlo by použít i `ctime()`, ta ale vrací pevný formát
`Wed Jun 30 21:49:08 1993` i s koncovým `\n`, který by se musel odstranit.

**Proč u velikosti `%12lld` a přetypování na `long long`?**
`st_size` je typu `off_t`, jehož přesná šířka není standardem daná. Přetypování
na `long long` a `%lld` je proto vždy správně. Číslo `12` zarovná velikosti
pod sebe do sloupce.

**Kdo rozvine hvězdičku v `./myls *.cpp`?**
Shell, ještě než program vůbec spustí. Program dostane v `argv` už hotový
seznam jmen a sám žádný adresář neprochází.

---

## gennum

**Jak se program ukončí pomocí `Ctrl-C`?**
`Ctrl-C` pošle procesu signál `SIGINT`. Mám na něj zaregistrovanou obsluhu
přes `sigaction()`, která jen nastaví příznak `running = 0`. Hlavní smyčka ho
při další kontrole uvidí a skončí — dopíše rozdělaný řádek a vypíše souhrn.

**Proč `volatile sig_atomic_t` a ne obyčejný `int`?**
`volatile` zabrání překladači uložit proměnnou do registru a smyčku
zoptimalizovat tak, že by změnu nikdy nezaznamenala. `sig_atomic_t` je typ,
u kterého standard garantuje zápis jednou nedělitelnou operací — jinak by se
mohlo stát, že signál přijde uprostřed zápisu.

**Proč `sigaction()` a ne jednodušší `signal()`?**
`signal()` má historicky různé chování na různých systémech — někde se po
doručení signálu obsluha automaticky zruší. `sigaction()` je přesně
definovaná a umožní navíc nastavit masku a příznaky.

**Co dělá příznak `SA_RESTART` a proč ho nenastavuješ?**
Se `SA_RESTART` se systémové volání přerušené signálem automaticky spustí
znovu. Já ho schválně nenastavuju, aby se spánek signálem opravdu přerušil
a program mohl skončit hned, ne až po dopočítání celého intervalu.

**Proč voláš `fflush( stdout )` po každém řádku?**
Když výstup nejde na terminál, ale do souboru nebo do roury, je `stdout` plně
bufferovaný — data se zapíšou až po naplnění zhruba 4 kB. `tail -f` ani
`monitor` by tak dlouho neměly co ukazovat. Na terminálu je `stdout`
bufferovaný po řádcích, takže tam se problém neprojeví.

**Šlo by to vyřešit jinak?**
Ano, `setvbuf( stdout, NULL, _IOLBF, 0 )` hned na začátku programu přepne
`stdout` na řádkové bufferování natrvalo. `fflush()` po každém řádku je ale
názornější — je na první pohled vidět, co a proč se děje.

**Jak počítáš, kdy vypsat další řádek?**
Interval je `60 / N` sekund. Čas dalšího řádku počítám **absolutně** od startu —
k cílovému času přičtu interval a spím pomocí `clock_nanosleep()` s příznakem
`TIMER_ABSTIME` až do toho okamžiku.

**Proč absolutně a ne prostě „počkej interval"?**
Protože k intervalu by se pokaždé přičetla ještě doba samotného výpisu
a generátor by se postupně zpožďoval. Při absolutním cíli se případné zdržení
jednoho kroku samo srovná v kroku dalším.

**Co vrací `clock_nanosleep()` a jak se liší od ostatních volání?**
Vrací chybový kód **přímo jako návratovou hodnotu**, ne přes `errno`. To je
proti zvyklostem, takže je to snadný zdroj chyb. Ošetřuju `EINTR` (přerušení
signálem — dospím zbytek) i ostatní chyby, aby program neskončil
v nekonečné smyčce bez čekání.

**Proč se do generátoru počtu čísel dává `1 + rand() % M`?**
Zadání říká, že `M` je *maximální* počet čísel na řádku. `rand() % M` dá
0 až M-1, přičtením jedničky dostanu 1 až M — prázdný řádek tak nikdy nevznikne.

---

## monitor

**Proč soubor nezavíráš?**
Protože otevřený deskriptor drží i-uzel naživu a udržuje pozici čtení. Kdybych
soubor po každé kontrole zavíral a znovu otvíral, ztratil bych pozici a hlavně
bych po smazání souboru přišel o data — takhle čtu dál i ze souboru, který už
nemá žádné jméno.

**Proč `fstat()` a ne `stat()`?**
`stat()` pracuje se jménem, a to může mezitím ukazovat na úplně jiný soubor —
třeba po přejmenování nebo při rotaci logů. `fstat()` se ptá na i-uzel, který
mám opravdu otevřený.

**Jak poznáš, že byl soubor zkrácen?**
Když je `st_size` menší než pozice, kam jsem se dočetl. Pak vypíšu hlášení
a skočím `lseek( fd, 0, SEEK_SET )` zpátky na začátek.

**A jak poznáš, že soubor narostl?**
Když je `st_size` větší než přečtená pozice. Pak čtu `read()`em ve smyčce,
dokud nevrátí 0, a o přečtené bajty posunu pozici.

**Proč hlášení o změnách posíláš na `stderr`?**
Aby se nemíchala do obsahu souboru. Díky tomu můžu spustit
`./monitor out.txt > kopie.txt` a dostanu čistou kopii, zatímco hlášení
zůstanou vidět na terminálu.

**Co se stane, když někdo soubor smaže?**
Nic dramatického — deskriptor zůstane platný a data jsou dál čitelná. Poznám to
podle `st_nlink == 0`, což znamená, že na i-uzel už nevede žádné jméno. I-uzel
se uvolní až po zavření posledního deskriptoru. Ověřit se to dá pohledem do
`/proc/PID/fd`, kde se u takového souboru objeví `(deleted)`.

**Proč ošetřuješ `EINTR` u `read()`?**
Protože `read()` může přerušit signál dřív, než stihne přečíst cokoliv. To
není chyba — stačí zavolat `read()` znovu. Bez tohoto ošetření by `Ctrl-C`
v nevhodnou chvíli vypadal jako chyba čtení.

**Proč kontroluješ jen jednou za sekundu a ne pořád dokola?**
Neustálé dotazování by zbytečně vytěžovalo procesor. Jednou za sekundu je
kompromis mezi rychlostí reakce a zátěží — a přesně to zadání požaduje.

**Existuje lepší způsob než opakované dotazování?**
Ano, `inotify` — jádro samo oznámí, že se soubor změnil, a program mezitím
vůbec neběží. Zadání ale výslovně chce `stat`/`fstat` a `lseek`, na kterých
je vidět, jak se se souborem pracuje na úrovni systémových volání.

**Proč `monitor` po `Ctrl-C` nekončí okamžitě?**
Obsluha signálu jen nastaví příznak. Program může být zrovna uprostřed
`sleep( 1 )` — ten se sice signálem přeruší, ale pak se ještě dokončí zbytek
cyklu. Ukončení tedy trvá nejvýš zhruba sekundu.

---

## Obecné

**Co je systémové volání a čím se liší od knihovní funkce?**
Systémové volání je požadavek na jádro — přepne procesor do režimu jádra
(`open`, `read`, `write`, `lseek`, `fstat`). Knihovní funkce běží v uživatelském
režimu a systémová volání si podle potřeby zavolá sama (`fopen`, `fread`,
`printf`); navíc si drží vlastní buffer.

**Proč tedy míchat `read()` a `fwrite()` v jednom programu?**
Vstup čtu `read()`em, protože nad ním potřebuju přesnou kontrolu pozice
pomocí `lseek()`. Výstup píšu `fwrite()`em, protože tam mi bufferování
naopak pomáhá — a řídím ho `fflush()`em.

**Co je i-uzel?**
Datová struktura popisující soubor — práva, vlastník, velikost, časy a odkazy
na datové bloky. **Jméno v ní není**; jméno je záznam v adresáři, který na
i-uzel ukazuje. Proto může mít jeden soubor víc jmen a proto po smazání
posledního jména soubor existuje dál, dokud ho má někdo otevřený.

**Co udělá `truncate -s 0`?**
Nastaví velikost souboru na nulu a uvolní jeho datové bloky, ale i-uzel
zůstává — je to pořád tentýž soubor. Proto ho monitor pozná podle poklesu
velikosti a nemusí nic znovu otvírat.
