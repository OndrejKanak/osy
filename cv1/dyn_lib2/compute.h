// ============================================================================
// compute.h - SPOLECNE rozhrani obou DYNAMICKYCH knihoven libcompute.so
//
// Tento soubor je ZAMERNE naprosto stejny v adresarich dyn_lib1 i dyn_lib2.
// Obe knihovny se jmenuji stejne (libcompute.so) a nabizeji stejnou funkci
// compute() - lisi se pouze tim, CO uvnitr dela.
//
//   dyn_lib1 ... k cislum na kazdem radku dopocita a pripoji jejich soucet
//   dyn_lib2 ... kontroluje, zda posledni cislo na radku odpovida souctu
//                vsech cisel pred nim
//
// Program main2 se preklada vzdy jen s JEDNOU z nich; ktera se skutecne
// pouzije, se rozhodne az pri spusteni podle promenne LD_LIBRARY_PATH.
// ============================================================================

#ifndef COMPUTE_H
#define COMPUTE_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// Zpracuje cely vstupni proud in (radek po radku) a vysledek vypise
// na standardni vystup.
//
// Navratova hodnota (stejna dohoda pro obe knihovny):
//     0  ... vse probehlo v poradku
//   > 0  ... pocet vadnych radku (vyuziva dyn_lib2 pri kontrole)
//    -1  ... chyba pri cteni vstupu
int compute( FILE *in );

#ifdef __cplusplus
}
#endif

#endif // COMPUTE_H
