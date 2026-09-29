// ============================================================================
// generator.h - rozhrani STATICKE knihovny libgenerator.a
//
// Knihovna umi generovat nahodna cela cisla v rozsahu <GEN_MIN, GEN_MAX>,
// tedy zamerne nikdy ne jednociferna.
//
// Predmet: Operacni systemy
// ============================================================================

#ifndef GENERATOR_H
#define GENERATOR_H

#include <stdio.h>

// Hlavicka je psana tak, aby sla pouzit z C i z C++.
// extern "C" vypne C++ name mangling - symbol se v knihovne jmenuje presne
// tak, jak je napsan ve zdrojaku (overitelne prikazem: nm libgenerator.a).
#ifdef __cplusplus
extern "C" {
#endif

// Povoleny rozsah generovanych cisel (obe meze vcetne).
#define GEN_MIN   10
#define GEN_MAX 1000

// Inicializace generatoru.
// seed == 0 -> odvodi se z aktualniho casu a PID procesu
//              (diky PID dostanou ruzne vysledky i procesy spustene
//               ve stejne sekunde, napr. v roure nebo ve smycce).
void gen_init( unsigned int seed );

// Vrati jedno nahodne cislo z rozsahu <GEN_MIN, GEN_MAX>.
int gen_number( void );

// Vypise na proud out jeden radek s 'count' cisly oddelenymi mezerou
// a ukonci ho znakem '\n'.
void gen_line( FILE *out, int count );

// Vypise na proud out 'rows' radku, kazdy s 'cols' cisly.
void gen_matrix( FILE *out, int rows, int cols );

#ifdef __cplusplus
}
#endif

#endif // GENERATOR_H
