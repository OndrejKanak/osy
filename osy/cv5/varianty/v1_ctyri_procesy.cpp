// ============================================================================
// v1_ctyri_procesy.cpp - varianta 1: kolona prodlouzena o ctvrty proces
//
// Pouziti:  ./v1_ctyri_procesy soubor [N]
//
// Dela totez co:
//
//     sort < soubor | nl -s ". " | tr a-z A-Z | head -n N      (N = 10)
//
//   soubor -> sort -r1-> nl -r2-> tr -r3-> head -> stdout
//
// Proti kolona1 pribyla roura 3 a potomek 4. Vsichni potomci po dup2()
// zavrou VSECHNY konce vsech rour - potrebne deskriptory uz maji jako
// 0 a 1. Proto staci jedna funkce zavri_roury() pro vsechny.
//
// Test:     sort < names.txt | nl -s ". " | tr a-z A-Z | head -n 10 > out1
//           ./v1_ctyri_procesy names.txt 10 > out2
//           diff out1 out2
//
// Predmet: Operacni systemy
// ============================================================================

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>       // fork, pipe, dup2, close, execlp
#include <fcntl.h>        // open
#include <sys/wait.h>     // wait

#define POCET_ROUR  3

static int roury[ POCET_ROUR ][ 2 ];

// Zavre oba konce vsech rour.
static void zavri_roury( void )
{
    for ( int i = 0; i < POCET_ROUR; i++ )
    {
        close( roury[ i ][ 0 ] );
        close( roury[ i ][ 1 ] );
    }
}

int main( int argc, char **argv )
{
    if ( argc != 2 && argc != 3 )
    {
        fprintf( stderr, "Pouziti: %s soubor [N]\n", argv[ 0 ] );
        return 1;
    }

    const char *n = ( argc == 3 ) ? argv[ 2 ] : "10";

    for ( int i = 0; i < POCET_ROUR; i++ )
    {
        if ( pipe( roury[ i ] ) < 0 )
        {
            perror( "pipe" );
            return 2;
        }
    }

    // ===================== POTOMEK 1: sort < soubor ========================
    if ( fork() == 0 )
    {
        int x = open( argv[ 1 ], O_RDONLY );

        if ( x < 0 )
        {
            perror( argv[ 1 ] );
            exit( 1 );
        }

        dup2( x, 0 );
        dup2( roury[ 0 ][ 1 ], 1 );
        close( x );
        zavri_roury();

        execlp( "sort", "sort", NULL );
        perror( "sort" );
        exit( 127 );
    }

    // ===================== POTOMEK 2: nl -s ". " ===========================
    if ( fork() == 0 )
    {
        dup2( roury[ 0 ][ 0 ], 0 );
        dup2( roury[ 1 ][ 1 ], 1 );
        zavri_roury();

        execlp( "nl", "nl", "-s", ". ", NULL );
        perror( "nl" );
        exit( 127 );
    }

    // ===================== POTOMEK 3: tr a-z A-Z ===========================
    if ( fork() == 0 )
    {
        dup2( roury[ 1 ][ 0 ], 0 );
        dup2( roury[ 2 ][ 1 ], 1 );
        zavri_roury();

        execlp( "tr", "tr", "a-z", "A-Z", NULL );
        perror( "tr" );
        exit( 127 );
    }

    // ===================== POTOMEK 4: head -n N ============================
    if ( fork() == 0 )
    {
        dup2( roury[ 2 ][ 0 ], 0 );     // stdout zustava
        zavri_roury();

        execlp( "head", "head", "-n", n, NULL );
        perror( "head" );
        exit( 127 );
    }

    // ===================== RODIC ===========================================
    zavri_roury();

    for ( int i = 0; i < 4; i++ )
        wait( NULL );

    return 0;
}
