// ============================================================================
// main2.cpp - program main2
//
// Sam o sobe nedela nic jineho, nez ze otevre vstup a preda ho funkci
// compute() z DYNAMICKE knihovny libcompute.so.
//
// Pouziti:
//     ./main2              ... cte ze standardniho vstupu
//     ./main2 soubor.txt   ... cte ze zadaneho souboru
//
// Ktera z obou stejnojmennych knihoven libcompute.so se skutecne pouzije,
// urcuje az pri spusteni promenna prostredi LD_LIBRARY_PATH:
//
//     LD_LIBRARY_PATH=../dyn_lib1 ./main2 < data.txt > data_se_soucty.txt
//     LD_LIBRARY_PATH=../dyn_lib2 ./main2 < data_se_soucty.txt
//
// Navratovy kod: 0 = vse v poradku
//                1 = nalezeny vadne radky
//                2 = chyba pri otevirani nebo cteni vstupu
// ============================================================================

#include <stdio.h>

#include "compute.h"   // nalezeno diky prepinaci -I<adresar knihovny>

int main( int argc, char **argv )
{
    if ( argc > 2 )
    {
        fprintf( stderr, "Pouziti: %s [soubor]\n", argv[ 0 ] );
        fprintf( stderr, "   bez argumentu se cte ze standardniho vstupu\n" );
        return 2;
    }

    FILE *in = stdin;

    if ( argc == 2 )
    {
        in = fopen( argv[ 1 ], "r" );

        if ( in == NULL )
        {
            perror( argv[ 1 ] );
            return 2;
        }
    }

    int result = compute( in );     // funkce z dynamicke knihovny

    if ( in != stdin )
        fclose( in );

    if ( result < 0 )
    {
        fprintf( stderr, "Chyba pri cteni vstupu.\n" );
        return 2;
    }

    return result == 0 ? 0 : 1;
}
