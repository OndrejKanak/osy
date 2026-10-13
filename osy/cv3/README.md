# Operační systémy — třetí cvičení: `myls` = `ls` + `stat`

Vychází z přípravy (`cv2/myls`). Program teď soubory nevypíše jednou, ale
**opakovaně každé 2 sekundy**, třídí je a hlídá změny jejich velikosti.

---

## 1. Struktura

```
cv3/
├── Makefile          přeloží myls
├── run_demo.sh       projede ukázkový scénář
├── README.md         tento soubor
├── OBHAJOBA.md       otázky k obhajobě i s odpověďmi
└── myls/myls.cpp     opakovaný výpis + hlídání změn
```

```bash
cd ~/osy/cv3 && chmod +x run_demo.sh && make
```

---

## 2. Použití

```bash
./myls/myls [-s] [-t] [-r] [-S | -N] [-u] soubor ... 2>/dev/pts/X
```

| přepínač | význam |
|---|---|
| `-s` | sloupec: velikost v bajtech |
| `-t` | sloupec: čas poslední změny obsahu |
| `-r` | sloupec: přístupová práva `-rw-r--r--` |
| `-S` | třídit podle **velikosti** (od nejmenšího) |
| `-N` | třídit podle **jména** (abecedně) |
| `-u` | **opačný** směr třídění |

- Sloupce se vypisují v pořadí, v jakém jsou přepínače zadány (jako v přípravě).
  Jméno je vždy poslední sloupec.
- Bez `-S`/`-N` zůstává pořadí, v jakém byly soubory zadány (`-u` ho obrátí).
- Soubory, které neexistují, jsou vždy na konci výpisu.
- Konec pomocí `Ctrl-C`.

Když jde `stdout` na terminál, obrazovka se před každým výpisem smaže (jako
`watch`). Při přesměrování do souboru se výpisy jen oddělí prázdným řádkem.

### Ukázka výpisu (`-S -u -s -r -t`)

```
=== 2026-09-29 07:59:12  trideni: velikost, obracene  (Ctrl-C = konec)
          35 ---------- 2026-09-29 07:59:05 b.txt   <-- NELZE CIST
          27 -rw-r--r-- 2026-09-29 07:58:59 a.txt
          10 -rw-r--r-- 2026-09-29 07:59:08 c.txt
           ? ?????????? ????-??-?? ??:??:?? nic.txt   <-- NENALEZEN
```

- **soubor zmizel** → všechny údaje jsou otazníky
- **soubor nejde číst** → na konci řádku `<-- NELZE CIST`

### Co jde na `stderr`

Když se při výpisu zjistí, že se změnila velikost souboru, vypíše se:

```
----- a.txt
pridany radek
----- b.txt
bez odradkovani
----- a.txt
(soubor zkracen: 27 -> 3 B, zadna nova data)
```

Za `----- jméno` následují **jen nová data** — bajty od původní do nové
velikosti. Při zkrácení žádná nová data nejsou, vypíše se jen poznámka.
Když soubor zmizí a znovu se objeví, je celý jeho obsah nový.

---

## 3. Dva terminály

**Terminál 2** — zjistit jméno terminálu:

```bash
tty
```

```
/dev/pts/3
```

**Terminál 1** — spustit `myls` a `stderr` poslat do terminálu 2:

```bash
cd ~/osy/cv3 && ./myls/myls -S -s -r -t *.txt 2>/dev/pts/3
```

**Terminál 3** (nebo třeba `gennum` z cv2) — měnit soubory:

```bash
echo "novy radek" >> a.txt      # v terminálu 2 se objeví  ----- a.txt  a nový řádek
chmod 000 b.txt                 # ve výpisu  <-- NELZE CIST
rm c.txt                        # ve výpisu otazníky
../cv2/gennum/gennum 5 60 >> a.txt
```

---

## 4. Jak to uvnitř funguje

Pro každý soubor si program pamatuje jen **jméno a poslední známou velikost**
(`known`). Každé kolo:

1. `stat()` na jméno — zjistí, jestli soubor existuje, a jeho údaje.
2. `access( jmeno, R_OK )` — zjistí, jestli ho smíme číst.
3. Když se velikost liší od `known`:
   `open()` → `lseek( fd, known )` → `read()` až do nové velikosti → `close()`.
   Data jdou na `stderr`. Pak `known = nová velikost`.
4. `qsort()` celé pole podle zvoleného klíče a výpis na `stdout`.
5. `sleep( 2 )`.

**Soubory se nedrží otevřené.** Mezi koly není otevřený žádný deskriptor
(ověř `ls -l /proc/$(pgrep myls)/fd` — jsou tam jen 0, 1, 2). To je rozdíl
proti `monitor` z cv2, který soubor držel otevřený pořád.

Při prvním výpisu se jen zapamatují výchozí velikosti — „nová" jsou až data,
která přibudou během sledování.

### Proč `stat()` a ne `lstat()`

V přípravě byl `lstat()`, aby symlink ukázal typ `l`. Tady ale sledujeme
**obsah** souborů a ten `open()` čte přes symbolický odkaz z cíle. Velikost
proto musí být také z cíle, jinak by se porovnávala délka cesty v odkazu
s daty v cílovém souboru.

### Pozor na roota

`access()` i `open()` pro roota projdou vždy, bez ohledu na práva. `<-- NELZE
CIST` se tedy ukáže jen u běžného uživatele.

---

## 5. Omezení

- Kontrola probíhá jednou za 2 s. Když se soubor během té doby zkrátí
  a zase naroste nad původní velikost, program to pozná jen jako růst
  a vypíše data od původní pozice.
- Když se soubor zkrátí, žádná „nová data" nejsou — jen poznámka.
- U adresářů a jiných neobyčejných souborů se při změně velikosti vypíše
  jen hlášení, obsah se nečte.
