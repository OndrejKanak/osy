// ============================================================================
// kolona2.cpp - 2. krok: druhy potomek cisluje radky sam (misto nl)
//
// Pouziti:  ./kolona2 soubor
//
//     sort < soubor | [ vlastni nl -s ". " ] | tr a-z A-Z
//
// Prvni a treti potomek jsou stejne jako v kolona1. Druhy potomek uz
// nespousti program nl - jeho exec je zakomentovany a misto nej je funkce
// cisluj_radky(), ktera dela presne totez co  nl -s ". " :
//
//     - neprazdny radek:  cislo zarovnane vpravo na 6 mist, ". ", radek
//                         "     1. anna"
//     - prazdny radek:    necisluje se, vypise se 8 mezer (6 + delka ". ")
//     - radek z mezer:    neni prazdny, cisluje se
//     - posledni radek bez '\n' na konci: '\n' se doplni
//
// Dulezite je spravne zpracovani radku: z roury mohou data prijit po
// libovolnych kusech, radek muze byt rozdeleny mezi dve cteni nebo jich
// muze prijit vic najednou. Proto se cte znak po znaku (getchar) a nikde
// neni zadny limit na delku radku.
//
// Test:     sort < names.txt | nl -s ". " | tr a-z A-Z > out1
//           ./kolona2 names.txt > out2
//           diff out1 out2               (nesmi nic vypsat)
//
// Predmet: Operacni systemy
// ============================================================================

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>       // fork, pipe, dup2, close, execlp
#include <fcntl.h>        // open
#include <sys/wait.h>     // waitpid

// ---------------------------------------------------------------------------
// Vlastni  nl -s ". "  : cte stdin, pise stdout.
//
// Stav 'zacatek_radku' rika, jestli dalsi znak je prvni znak noveho radku.
// Jen v tu chvili se rozhoduje, jestli se vypise cislo (radek ma obsah),
// nebo jen mezery (radek je prazdny - prvni znak je rovnou '\n').
// ---------------------------------------------------------------------------
static void cisluj_radky( void )
{
    int cislo         = 0;
    int zacatek_radku = 1;
    int c;

    while ( ( c = getchar() ) != EOF )
    {
        if ( zacatek_radku )
        {
            if ( c == '\n' )
                printf( "%6s  ", "" );              // prazdny radek: 8 mezer
            else
                printf( "%6d. ", ++cislo );         // cislo + oddelovac
        }

        putchar( c );
        zacatek_radku = ( c == '\n' );
    }

    if ( !zacatek_radku )       // posledni radek nemel '\n' - doplnime ho
        putchar( '\n' );
}

int main( int argc, char **argv )
{
    if ( argc != 2 )
    {
        fprintf( stderr, "Pouziti: %s soubor\n", argv[ 0 ] );
        return 1;
    }

    int roura1[ 2 ];                // sort -> potomek 2
    int roura2[ 2 ];                // potomek 2 -> tr

    if ( pipe( roura1 ) < 0 || pipe( roura2 ) < 0 )
    {
        perror( "pipe" );
        return 2;
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

        dup2( x, 0 );               // stdin  = soubor
        dup2( roura1[ 1 ], 1 );     // stdout = roura 1

        close( x );
        close( roura1[ 0 ] );
        close( roura1[ 1 ] );
        close( roura2[ 0 ] );
        close( roura2[ 1 ] );

        execlp( "sort", "sort", NULL );
        perror( "sort" );
        exit( 1 );
    }

    // ===================== POTOMEK 2: vlastni cislovani radku ==============
    if ( fork() == 0 )
    {
        dup2( roura1[ 0 ], 0 );     // stdin  = roura 1
        dup2( roura2[ 1 ], 1 );     // stdout = roura 2

        close( roura1[ 0 ] );
        close( roura1[ 1 ] );
        close( roura2[ 0 ] );
        close( roura2[ 1 ] );

        // Puvodni reseni z kolona1:
        // execlp( "nl", "nl", "-s", ". ", NULL );
        // perror( "nl" );
        // exit( 1 );

        cisluj_radky();

        exit( 0 );                  // exit() vyprazdni stdout buffer do roury
    }

    // ===================== POTOMEK 3: tr a-z A-Z ===========================
    if ( fork() == 0 )
    {
        dup2( roura2[ 0 ], 0 );     // stdin  = roura 2, stdout zustava

        close( roura1[ 0 ] );
        close( roura1[ 1 ] );
        close( roura2[ 0 ] );
        close( roura2[ 1 ] );

        execlp( "tr", "tr", "a-z", "A-Z", NULL );
        perror( "tr" );
        exit( 1 );
    }

    // ===================== RODIC: zavre roury a ceka =======================
    close( roura1[ 0 ] );
    close( roura1[ 1 ] );
    close( roura2[ 0 ] );
    close( roura2[ 1 ] );

    for ( int i = 0; i < 3; i++ )
        wait( NULL );

    return 0;
}
