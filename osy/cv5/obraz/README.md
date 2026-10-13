# Voynichův rukopis — oprava poškozeného obrázku

Virus převedl obrázek do textové hexadecimální podoby, řádky očísloval,
zamíchal a každý řádek otočil. Původní obrázek vrátí příkaz:

```bash
rev < bad.jpg | sort -n | cut -d . -f 2 | basenc -i -d --base16 > img.jpg
```

Programy v této složce dělají totéž přes `fork`, `pipe`, `dup2` a `exec`,
postupně podle čtyř bodů zadání.

| soubor | bod zadání | kolona |
|---|---|---|
| `obraz1.cpp` | 1 | `rev \| sort -n \| cut -d . -f 2 \| basenc -i -d --base16` |
| `myrev.cpp` | 2 | vlastní program `rev` |
| `obraz2.cpp` | 2 | `./myrev \| sort -n \| cut \| basenc` |
| `obraz3.cpp` | 3 | `./myrev \| sort -n \| cut \| vlastní převod hex → bajty` |
| `obraz4.cpp` | 4 | `./myrev \| sort -n \| cut \| vlastní převod \| display -` |

---

## 1. Jak vypadá poškozený soubor

Jeden řádek `bad.jpg` (mezery na konci jsou součástí dat):

```
 B 6 A D 7 7 7 1 B 1 1 1 5 2 B 9 B 6 A A E B 3 E 4 2 9 1 3 2 7 2 5 8 1 F C 1 4 9 .0877  
```

Co s ním dělá každý krok kolony:

| krok | výsledek | proč |
|---|---|---|
| `rev` | `  7780. 9 4 1 C F 1 8 5 ...` | řádek se otočí: číslo řádku je zase na začátku, hex číslice ve správném pořadí |
| `sort -n` | `     1. F F D 8 F F E 0 ...` | řádky se seřadí podle čísla na začátku |
| `cut -d . -f 2` | ` F F D 8 F F E 0 ...` | vezme se jen část za tečkou — zahodí se číslo řádku |
| `basenc -i -d --base16` | bajty `FF D8 FF E0 ...` | dvojice hex číslic → jeden bajt; `-i` ignoruje mezery a konce řádků |

Výsledek začíná `FF D8` (začátek každého JPEG) a končí `FF D9` (konec JPEG).

---

## 2. Překlad

```bash
cd obraz
make
```

Vzniknou programy `myrev`, `obraz1`, `obraz2`, `obraz3`, `obraz4`.

**Důležité:** `obraz2`, `obraz3` a `obraz4` spouštějí `./myrev`, tedy
`myrev` **z aktuální složky**. Spouštějte je proto ve složce `obraz`
(tam, kde je `myrev`). Z jiné složky skončí hláškou
`./myrev: No such file or directory`.

---

## 3. Spuštění a testování

Referenční výsledek:

```bash
rev < bad.jpg | sort -n | cut -d . -f 2 | basenc -i -d --base16 > img.jpg
```

### Bod 1 — `obraz1`: čtyři příkazy

```bash
./obraz1 bad.jpg img1.jpg
cmp img.jpg img1.jpg
```

Bez druhého parametru jde výsledek na stdout:

```bash
./obraz1 bad.jpg > img1.jpg
```

### Bod 2 — `myrev` a `obraz2`

Samotný `myrev` proti `rev`:

```bash
rev < bad.jpg > r1
./myrev < bad.jpg > r2
cmp r1 r2
echo "ahoj svete" | ./myrev          # etevs joha
```

Kolona s vlastním `myrev`:

```bash
./obraz2 bad.jpg img2.jpg
cmp img.jpg img2.jpg
```

### Bod 3 — `obraz3`: vlastní převod hex → bajty

```bash
./obraz3 bad.jpg img3.jpg
cmp img.jpg img3.jpg
```

### Bod 4 — `obraz4`: zobrazení

```bash
./obraz4 bad.jpg
```

Otevře se okno programu `display` s obrázkem. Zavřete ho (klávesa `q`
nebo křížek) a program skončí.

### Kontrola formátu

```bash
hexdump -C img3.jpg | head -3
hexdump -C img3.jpg | tail -2
file img3.jpg
```

```
00000000  ff d8 ff e0 00 10 4a 46  49 46 00 01 01 00 00 01  |......JFIF......|
...
img3.jpg: JPEG image data, JFIF standard 1.01, ... 1280x867, components 3
```

`cmp` nevypíše nic, když jsou soubory shodné.

---

## 4. Jak programy fungují

### Společná kostra (`obraz1` až `obraz4`)

Rodič vytvoří roury, pak potomky. Každý potomek:

1. přesměruje stdin a stdout pomocí `dup2` — na soubor nebo na rouru,
2. zavře **všechny** konce všech rour funkcí `zavri_roury()` — potřebné
   už má zkopírované na deskriptorech 0 a 1,
3. spustí program přes `execlp`, nebo zavolá vlastní funkci a `exit( 0 )`.

Rodič zavře všechny roury a počká na všechny potomky (`wait`).

```
 bad.jpg ─▶ [P1 rev] ─r0─▶ [P2 sort -n] ─r1─▶ [P3 cut] ─r2─▶ [P4 basenc] ─▶ img.jpg / stdout
```

| potomek | stdin (0) | stdout (1) |
|---|---|---|
| 1 | `bad.jpg` (`open` + `dup2`) | roura 0 |
| 2 | roura 0 | roura 1 |
| 3 | roura 1 | roura 2 |
| 4 | roura 2 | soubor z 2. parametru, jinak obrazovka |

Argumenty pro `execlp` se píšou každý zvlášť, bez uvozovek shellu:

```
cut -d . -f 2               →  execlp( "cut", "cut", "-d", ".", "-f", "2", NULL )
basenc -i -d --base16       →  execlp( "basenc", "basenc", "-i", "-d", "--base16", NULL )
```

### `myrev` — vlastní `rev`

Čte řádky funkcí `getline` (sama si zvětšuje buffer, délka řádku není
omezená). U každého řádku:

1. zjistí, jestli končí `\n`, a pokud ano, `\n` z otáčení vynechá,
2. vypíše znaky od posledního k prvnímu,
3. vrátí `\n` na konec — jen když tam původně byl.

Díky tomu se chová stejně jako `rev` i u prázdného řádku a u posledního
řádku bez `\n`.

V `obraz2` je v prvním potomkovi místo `execlp( "rev", ... )`:

```cpp
execlp( "./myrev", "myrev", NULL );
```

`execlp` s `/` ve jméně nehledá v `PATH`, ale vezme přímo zadanou cestu —
proto `./myrev` z aktuální složky.

### Vlastní převod hex → bajty (`obraz3`, `obraz4`)

Čtvrtý potomek místo `basenc` volá funkci `hex_na_bajty()`:

- čte znak po znaku (`getchar`),
- `hodnota_hex( c )` vrátí hodnotu 0–15 pro `0`–`9`, `A`–`F`, `a`–`f`,
  jinak −1,
- znaky, které nejsou hex číslice (mezery, `\n`), přeskočí — to je
  přepínač `-i` u `basenc`,
- první číslice dvojice se uloží do `horni`, s druhou se vypíše bajt
  `horni * 16 + dolni` a `horni` se vynuluje (−1).

Příklad: `F F D 8` → `F,F` → 15·16+15 = 255 = `0xFF`, `D,8` → 13·16+8 = 216 = `0xD8`.

Dvojice může začít na jednom řádku a skončit na dalším — nevadí to,
`horni` se mezi řádky pamatuje.

Výstup jde přes `putchar` (buffer stdio) a na konci `exit( 0 )` buffer
vyprázdní do roury nebo do souboru.

### Pátý potomek `display -` (`obraz4`)

Přibyla čtvrtá roura. Čtvrtý potomek teď píše do ní (`dup2( roury[3][1], 1 )`)
a pátý potomek si ji vezme jako stdin a spustí:

```cpp
execlp( "display", "display", "-", NULL );
```

`-` znamená „čti obrázek ze stdin“. `display` je z balíku ImageMagick
a potřebuje grafické prostředí.

---

## 5. Ověřené výsledky

| test | výsledek |
|---|---|
| `obraz1` → soubor i stdout | shodné s referencí (374 938 B) |
| `obraz2` (`./myrev`) | shodné |
| `obraz3` (vlastní hex) → soubor i stdout | shodné |
| `obraz4` — data předaná do `display -` | shodná |
| `myrev` proti `rev` na `bad.jpg` | shodné |
| `myrev` — prázdný řádek, poslední řádek bez `\n`, řádek 200 000 znaků | shodné s `rev` |
| otevřené roury za běhu `obraz4` | rodič 0, každý potomek jen svůj stdin a stdout |

Výsledný obrázek: JPEG 1280×867, dvoustrana Voynichova rukopisu s rostlinami.

---

## 6. Možné potíže

| hláška | příčina |
|---|---|
| `./myrev: No such file or directory` | program nespouštíte ve složce, kde je `myrev`, nebo jste nespustil `make` |
| `display: No such file or directory` | chybí ImageMagick — `sudo apt install imagemagick`, nebo použijte `obraz3 bad.jpg img.jpg` |
| `display: unable to open X server` | nejste v grafickém prostředí (např. přes ssh bez `-X`) |
| `basenc: No such file or directory` | starší coreutils (před 8.31); `obraz3` a `obraz4` `basenc` nepotřebují |
