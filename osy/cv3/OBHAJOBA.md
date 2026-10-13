# Otázky k obhajobě — třetí cvičení

Krátké odpovědi, tak jak se dají říct nahlas.

---

**Jak zjistíš, že se soubor změnil?**
Pro každý soubor si pamatuju poslední známou velikost. Každé kolo zavolám
`stat()` a porovnám `st_size` s uloženou hodnotou. Když se liší, soubor se
změnil.

**Jak vypíšeš jen nová data?**
Soubor otevřu `open()`, skočím `lseek()` na starou velikost a `read()` čtu až
do nové velikosti zjištěné `stat()`. Pak soubor hned zavřu `close()`.

**Proč soubor nedržíš otevřený?**
Zadání to chce — a má to smysl: otevřený deskriptor zabírá prostředek
v jádře, u mnoha souborů by došly (`ulimit -n`). Navíc by deskriptor ukazoval
na starý i-uzel, i kdyby soubor někdo smazal a vytvořil znovu. Přes jméno
vidím vždy aktuální stav.

**Jak poznáš, že soubor zmizel?**
`stat()` vrátí `-1` (`errno == ENOENT`). Pak místo údajů vypíšu otazníky
a uloženou velikost nastavím na 0, takže když se soubor znovu objeví, vypíše
se celý jeho obsah jako nový.

**Jak poznáš, že soubor nejde číst?**
`access( jmeno, R_OK )`. Testuje práva s reálným UID/GID procesu, tedy přesně
to, co by udělal `open()` pro čtení. Jako root to ale projde vždy.

**Šlo by to i bez `access()`?**
Ano — ze `st_mode`, `st_uid` a `st_gid` a porovnáním s `getuid()`/`getgid()`
a doplňkovými skupinami. To je ale přesně to, co `access()` dělá za mě, a ještě
by se muselo řešit ACL a root.

**Jak třídíš?**
`qsort()` s vlastní porovnávací funkcí. Podle přepínače porovnává
`st_size`, nebo `strcmp()` jmen. Při `-u` výsledek porovnání otočím
znaménkem. Chybějící soubory jsou vždy na konci, protože jejich velikost
neznáme.

**Proč se třídí každé kolo znovu?**
Velikosti se mezi koly mění, takže pořadí podle velikosti se mění taky.
Stav souboru (uložená velikost) je přímo ve struktuře, kterou `qsort()`
přesouvá, takže se nic neztratí.

**Proč `stat()` a ne `lstat()`?**
Sleduju obsah, a `open()` čte přes symlink z cílového souboru. Velikost
musí odpovídat tomu, co pak čtu.

**Jak přesměruješ `stderr` do jiného terminálu?**
Ve druhém terminálu příkazem `tty` zjistím jeho zařízení, třeba
`/dev/pts/3`. V prvním pak `./myls ... 2>/dev/pts/3`. Terminál je v Linuxu
obyčejný soubor, do kterého se dá zapisovat.

**Proč data na `stderr` zapisuješ přes `write()`?**
Jsou to libovolné bajty a nemusí to být text — `write()` je zapíše přesně.
Zbytek hlášení jde přes `fprintf( stderr, ... )`; `stderr` není bufferovaný,
takže se pořadí nepomíchá.

**Proč je po výpisu `fflush( stdout )`?**
Když se `stdout` přesměruje do souboru, je plně bufferovaný a výpisy by se
objevily až po naplnění bufferu. Stejná past jako u `gennum` v cv2.

**Co když se soubor zkrátí?**
Žádná nová data nejsou, vypíšu jen `----- soubor` a poznámku o zkrácení.
Uloženou velikost nastavím na novou, aby další přírůstek šel od ní.
