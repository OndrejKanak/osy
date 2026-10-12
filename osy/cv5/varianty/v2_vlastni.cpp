// ============================================================================
// v2_vlastni.cpp - varianta 2: prikazy nahrazene vlastnim kodem
//
// Pouziti:  ./v2_vlastni soubor filtr [argument]
//
//     filtr:  nl | tr | head N | grep TEXT | wc | uniq | sort
//
// Dela totez co:
//
//     sort < soubor | FILTR | tr a-z A-Z
//
//   soubor -> [ P1: sort (exec) ] -r1-> [ P2: vlastni FILTR ] -r2-> [ P3: vlastni tr ] -> stdout
//
// Prvni potomek porad spousti program sort. Druhy a treti uz zadny program
// nespousti - po stejnych dup2() jako v kolona1 zavolaji funkci z
// vlastni.hpp, ktera cte stdin a pise stdout.
//
// Test (kazdy filtr proti skutecnemu programu):
//     sort < names.txt | nl -s ". " | tr a-z A-Z > out1 ; ./v2_vlastni names.txt nl > out2 ; diff out1 out2
//     sort < names.txt | head -n 5  | tr a-z A-Z > out1 ; ./v2_vlastni names.txt head 5 > out2 ; diff out1 out2
//     sort < names.txt | grep -F an | tr a-z A-Z > out1 ; ./v2_vlastni names.txt grep an > out2 ; diff out1 out2
//     sort < names.txt | wc -l      | tr a-z A-Z > out1 ; ./v2_vlastni names.txt wc > out2     ; diff out1 out2
//     sort < names.txt | uniq       | tr a-z A-Z > out1 ; ./v2_vlastni names.txt uniq > out2   ; diff out1 out2
//
// Predmet: Operacni systemy
// ============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>       // fork, pipe, dup2, close, execlp
#include <fcntl.h>        // open
#include <sys/wait.h>     // wait

#include "vlastni.hpp"

static int roura1[ 2 ];
static int roura2[ 2 ];

static void zavri_roury( void )
{
    close( roura1[ 0 ] );
    close( roura1[ 1 ] );
    close( roura2[ 0 ] );
    close( roura2[ 1 ] );
}

// Spusti vybrany vlastni filtr. Vraci 0, kdyz filtr existuje.
static int spust_filtr( const char *filtr, const char *argument )
{
    if      ( strcmp( filtr, "nl" )   == 0 ) moje_nl();
    else if ( strcmp( filtr, "tr" )   == 0 ) moje_tr();
    else if ( strcmp( filtr, "wc" )   == 0 ) moje_wc_l();
    else if ( strcmp( filtr, "uniq" ) == 0 ) moje_uniq();
    else if ( strcmp( filtr, "sort" ) == 0 ) moje_sort();
    else if ( strcmp( filtr, "head" ) == 0 && argument ) moje_head( atoi( argument ) );
    else if ( strcmp( filtr, "grep" ) == 0 && argument ) moje_grep( argument );
    else return -1;

    return 0;
}

int main( int argc, char **argv )
{
    if ( argc != 3 && argc != 4 )
    {
        fprintf( stderr, "Pouziti: %s soubor filtr [argument]\n", argv[ 0 ] );
        fprintf( stderr, "   filtr: nl | tr | head N | grep TEXT | wc | uniq | sort\n" );
        return 1;
    }

    const char *filtr    = argv[ 2 ];
    const char *argument = ( argc == 4 ) ? argv[ 3 ] : NULL;

    if ( pipe( roura1 ) < 0 || pipe( roura2 ) < 0 )
    {
        perror( "pipe" );
        return 2;
    }

    // ===================== POTOMEK 1: sort < soubor (exec) =================
    if ( fork() == 0 )
    {
        int x = open( argv[ 1 ], O_RDONLY );

        if ( x < 0 )
        {
            perror( argv[ 1 ] );
            exit( 1 );
        }

        dup2( x, 0 );
        dup2( roura1[ 1 ], 1 );
        close( x );
        zavri_roury();

        execlp( "sort", "sort", NULL );
        perror( "sort" );
        exit( 127 );
    }

    // ===================== POTOMEK 2: vlastni filtr ========================
    if ( fork() == 0 )
    {
        dup2( roura1[ 0 ], 0 );
        dup2( roura2[ 1 ], 1 );
        zavri_roury();

        if ( spust_filtr( filtr, argument ) != 0 )
        {
            fprintf( stderr, "Neznamy filtr '%s' (nebo chybi argument)\n", filtr );
            exit( 1 );
        }

        exit( 0 );                  // exit() vyprazdni stdout buffer do roury
    }

    // ===================== POTOMEK 3: vlastni tr a-z A-Z ===================
    if ( fork() == 0 )
    {
        dup2( roura2[ 0 ], 0 );
        zavri_roury();

        // misto: execlp( "tr", "tr", "a-z", "A-Z", NULL );
        moje_tr();

        exit( 0 );
    }

    // ===================== RODIC ===========================================
    zavri_roury();

    for ( int i = 0; i < 3; i++ )
        wait( NULL );

    return 0;
}
