# Otázky k obhajobě — i s odpověďmi

Otázky jsou seřazené zhruba podle toho, jak pravděpodobně padnou.
Odpovědi jsou schválně krátké — tak, jak se dají říct nahlas.

---

## Knihovny obecně

**Co je statická knihovna?**
Archiv přeložených objektových souborů, nic víc. Vyrobí ji program `ar`,
uvnitř jsou `.o` soubory plus index symbolů. Při linkování programu z ní
linker **vykopíruje** jen ty funkce, které program opravdu volá, a vloží je
přímo do výsledné binárky.

**Co je dynamická (sdílená) knihovna?**
Skoro hotový program bez funkce `main()`, který se dá za běhu namapovat do
paměti procesu. Do programu se při linkování zapíše jen **jméno** knihovny
(záznam `NEEDED`), samotný kód v programu není. Knihovnu natahuje až při
spuštění dynamický linker `ld.so`.

**Jaké jsou výhody dynamických knihoven?**
Jednu kopii v paměti sdílí všechny procesy, které ji používají. Oprava
knihovny nevyžaduje překlad programů. A jak ukazuje toto cvičení, jde
vyměnit implementaci bez zásahu do programu.

**A nevýhody?**
Program se bez knihovny vůbec nespustí, je potřeba řešit, kde se hledá
(`LD_LIBRARY_PATH`, `ldconfig`), a start programu je o něco pomalejší,
protože se musí dohledat a navázat symboly.

---

## Překlad

**Proč `-fPIC`?**
Position Independent Code. Sdílená knihovna se v každém procesu může
namapovat na jinou adresu, takže kód nesmí obsahovat pevné absolutní adresy —
adresuje se relativně vůči sobě, přes tabulky GOT a PLT. Bez `-fPIC` linker
odmítne knihovnu vyrobit hláškou `recompile with -fPIC`.

**Proč `-shared`?**
Říká linkeru, že výstupem není spustitelný program, ale sdílená knihovna —
tedy že se nemá hledat `main()` a nemá se přidávat startovací kód.

**Co dělá `-I`, `-L` a `-l`?**
`-I` platí pro **preprocesor** při překladu `.cpp` na `.o` a říká, kde hledat
hlavičkové soubory. `-L` platí pro **linker** a říká, v jakém adresáři hledat
soubor knihovny. `-l` říká, **kterou** knihovnu použít.

**Proč se u `-l` píše `-lcompute` a ne `libcompute.so`?**
Protože linker si název složí sám: z `-lcompute` udělá `libcompute.so`
a když ji nenajde, zkusí `libcompute.a`. Proto se knihovna **musí** jmenovat
`lib` + jméno + přípona. Soubor `compute.so` linker nenajde a oznámí
`cannot find -lcompute`.

**Proč záleží na pořadí argumentů linkeru?**
Linker prochází argumenty zleva doprava a z knihovny vytáhne jen to, co mu
v danou chvíli chybí. Když je `-l` napsané **před** objektovými soubory,
v tu chvíli ještě nic nechybí, knihovna se přeskočí a skončí to na
`undefined reference`. Proto `g++ main1.o -L... -lgenerator -o main1`.

**Co dělá `ar rcs`?**
`r` vloží nebo nahradí soubory v archivu, `c` archiv vytvoří bez varování,
když ještě neexistuje, `s` vygeneruje index symbolů (dřív samostatný `ranlib`).

---

## Spouštění a LD_LIBRARY_PATH

**Jak dynamický linker hledá knihovny?**
V pořadí: `DT_RPATH` v programu → `LD_LIBRARY_PATH` → `DT_RUNPATH` v programu →
cache `/etc/ld.so.cache` → systémové adresáře `/lib`, `/usr/lib`, `/lib64`.

**Co je `LD_LIBRARY_PATH`?**
Proměnná prostředí se seznamem adresářů oddělených dvojtečkou, ve kterých má
`ld.so` hledat sdílené knihovny dřív než v systémových. Je to právě to místo,
kterým se v tomto cvičení vybírá jedna ze dvou knihoven.

**Proč musí být `export`?**
Bez `export` vznikne jen proměnná shellu. Nový proces dostává při `exec` pouze
prostředí (`environ`), interní proměnné shellu nezdědí — takže `ld.so` by
o cestě nevěděl. Alternativa je zápis `LD_LIBRARY_PATH=... ./main2` na jednom
řádku, což proměnnou vloží do prostředí právě toho jednoho příkazu.

**Co se stane, když program spustíš bez `LD_LIBRARY_PATH`?**
Skončí ještě před prvním řádkem `main()` hláškou
`error while loading shared libraries: libcompute.so: cannot open shared object file`.
Překlad přitom proběhl bez chyby — to je hlavní rozdíl oproti chybě linkeru.

**Proč se do programu neuloží i cesta ke knihovně?**
Uložila by se, kdybychom linkovali s `-Wl,-rpath`. To ale schválně neděláme,
protože `RPATH` má přednost před `LD_LIBRARY_PATH` a přepínání knihoven by
přestalo fungovat. Ověřit se to dá příkazem `readelf -d main2 | grep RPATH` —
žádný záznam tam není.

**Co je SONAME?**
Jméno, pod kterým se knihovna hlásí a pod kterým ji programy hledají
(`g++ -shared -Wl,-soname,libcompute.so.1`). Slouží k verzování: program
si zapamatuje `libcompute.so.1` a nespustí se s nekompatibilní verzí 2.
My SONAME nenastavujeme, takže se do programu zapíše prostě jméno souboru.

**Kdy přesně se rozhodne, která `compute()` se zavolá?**
Až při spuštění procesu. `ld.so` najde knihovnu podle `LD_LIBRARY_PATH`,
namapuje ji a naváže symbol `compute` do tabulky PLT/GOT. Ve výchozím
nastavení navíc líně (lazy binding) — adresa se doplní až při prvním volání.

---

## Tento konkrétní projekt

**Proč se obě dynamické knihovny jmenují stejně?**
To je celá pointa. Program `main2` má uložené jen jméno `libcompute.so`.
Když se obě soubory jmenují stejně a exportují stejnou funkci, jsou pro
program zaměnitelné a rozhodne až `LD_LIBRARY_PATH`.

**Proč `extern "C"` v hlavičce?**
C++ překladač jména funkcí "mangluje" — přidává do nich zakódované typy
parametrů. `extern "C"` to vypne, takže se symbol jmenuje přesně `compute`.
Knihovna je díky tomu použitelná i z C a je vidět čitelné jméno v `nm -D`.

**S kterou knihovnou je `main2` přeložený?**
S `../dyn_lib1`, ale je to jedno — kdyby byl přeložený s `dyn_lib2`, choval by
se úplně stejně. Do binárky se z `-L` nic neukládá, ta cesta slouží jen
linkeru při překladu.

**Potřebuje `main1` za běhu `libgenerator.a`?**
Ne. Kód se do něj při linkování zkopíroval. `ldd main1` ukáže jen systémové
knihovny (`libc`, `libstdc++`), o naší knihovně tam není zmínka. Soubor
`libgenerator.a` se dá po překladu klidně smazat a `main1` bude dál fungovat.

**Jak generuješ čísla v rozsahu 10 až 1000?**
`GEN_MIN + rand() % ( GEN_MAX - GEN_MIN + 1 )`. Modulo zúží `rand()` na šířku
intervalu (991 hodnot) a přičtení dolní meze ho posune. Tím je zaručeno,
že číslo nikdy nebude jednociferné.

**Proč se generátor inicializuje z času i z PID?**
Samotné `time(NULL)` má rozlišení na sekundy — dva programy spuštěné ve stejné
sekundě (třeba v rouře nebo ve smyčce) by generovaly stejná čísla. Doplnění
PID to rozliší.

**Jak kontrolní knihovna pozná, co je na řádku součet?**
Bere **poslední číslo na řádku** jako deklarovaný součet a porovná ho se
součtem všech čísel před ním. Nepotřebuje tedy vědět, kolik čísel na řádku
původně bylo. Řádek bez dopočteného součtu tím pádem spolehlivě neprojde.

**Jak čteš řádky, když dopředu nevíš, jak budou dlouhé?**
Pomocnou funkcí `read_line()`, která čte `fgets()`em do bufferu a při zaplnění
ho `realloc()`em zdvojnásobí. Dělá tedy totéž co POSIX `getline()`, ale je
přenositelná. Při `./main1 100 500` má řádek přes 1900 znaků a projde bez
problémů — buffer se zvětší sám.

**Proč ne rovnou POSIX `getline()`?**
Na Linuxu by fungovala úplně stejně a je to jednodušší zápis. Vlastní
`read_line()` má ale výhodu, že se kód přeloží i tam, kde `getline()` není
(třeba MinGW na Windows), a je na ní hezky vidět práce s dynamickou pamětí.

**Co vrací funkce `compute()`?**
Obě knihovny se drží stejné dohody: `0` = v pořádku, kladné číslo = počet
vadných řádků, `-1` = chyba čtení. `main2` to převede na návratový kód
procesu, takže se výsledek dá použít ve skriptu.

---

## Nástroje, kterými se dá všechno ukázat

| příkaz | co ukáže |
|---|---|
| `file libcompute.so` | že jde o sdílený objekt ELF |
| `ar t libgenerator.a` | seznam `.o` souborů uvnitř statické knihovny |
| `nm libgenerator.a` | symboly ve statické knihovně (`T` = definovaná funkce) |
| `nm -D --defined-only libcompute.so` | co dynamická knihovna nabízí |
| `nm -D --undefined-only main2` | co program od knihovny vyžaduje |
| `ldd main2` | které knihovny se natáhnou a odkud |
| `readelf -d main2` | záznamy `NEEDED`, `RPATH`, `RUNPATH` |
| `LD_DEBUG=libs ./main2` | celé hledání knihoven krok za krokem |

Nejsilnější ukázka u obhajoby jsou tyhle dva příkazy vedle sebe — stejný
program, dvě různé knihovny:

```bash
LD_LIBRARY_PATH=$PWD/dyn_lib1 ldd main2/main2 | grep compute
```

```bash
LD_LIBRARY_PATH=$PWD/dyn_lib2 ldd main2/main2 | grep compute
```
