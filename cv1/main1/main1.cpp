// ============================================================================
// main1.cpp - program 'main1' (generator dat)
//
// Pouziti:  ./main1 M N
//           M = pocet radku vystupu
//           N = pocet cisel na kazdem radku (kazde cislo 10 az 1000)
//
// Vystup jde na stdout, takze se da presmerovat do souboru:
//           ./main1 100 20 > data.txt
// nebo poslat rourou dalsimu programu:
//           ./main1 100 20 | ../main2/main2
//
// Vlastni generovani cisel je ve STATICKE knihovne libgenerator.a
// (adresar ../static_lib). Zde je pouze funkce main a kontrola argumentu.
// ============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "generator.h"   // nalezeno diky prepinaci -I../static_lib

// Prevede textovy argument na kladne cele cislo, jinak program ukonci.
static int parse_positive( const char *text, const char *name )
{
    char *end = NULL;

    errno = 0;
    long value = strtol( text, &end, 10 );

    if ( errno != 0 || end == text || *end != '\0' || value <= 0 )
    {
        fprintf( stderr, "Chyba: argument %s musi byt kladne cele cislo, dostal jsem '%s'.\n",
                 name, text );
        exit( 1 );
    }

    return (int) value;
}

int main( int argc, char **argv )
{
    if ( argc != 3 )
    {
        fprintf( stderr, "Pouziti: %s M N\n", argv[ 0 ] );
        fprintf( stderr, "   M = pocet radku\n" );
        fprintf( stderr, "   N = pocet cisel na radku (kazde v rozsahu %d az %d)\n",
                 GEN_MIN, GEN_MAX );
        fprintf( stderr, "Priklad: %s 100 20 > data.txt\n", argv[ 0 ] );
        return 1;
    }

    int rows = parse_positive( argv[ 1 ], "M" );
    int cols = parse_positive( argv[ 2 ], "N" );

    gen_init( 0 );                      // funkce ze staticke knihovny
    gen_matrix( stdout, rows, cols );   // funkce ze staticke knihovny

    return 0;
}
