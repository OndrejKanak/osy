// ============================================================================
// kalendar2.cpp - 2. verze: dva potomci, jedna roura
//
// Pouziti:  ./kalendar2 M N
//
//     M ... kolik datumu se vygeneruje
//     N ... kolik datumu za sekundu (tempo generovani, usleep)
//
//                     +-------+
//                     | RODIC |   jen vytvori potomky a ceka
//                     +---+---+
//                +--------+--------+
//                v                 v
//          +-----------+     +-----------+
//          | POTOMEK 1 | --> | POTOMEK 2 | ---> stdout
//          +-----------+     +-----------+
//            generuje   roura  doplni jmeno
//
// Kod, ktery v 1. verzi delal rodic, se presunul do druheho potomka.
// Rodic uz s rourou nepracuje - jen ji vytvori, preda potomkum, zavre
// a pocka na jejich ukonceni.
//
// Predmet: Operacni systemy
// ============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <unistd.h>       // fork, pipe, close, write, usleep
#include <sys/wait.h>     // waitpid

#include "svatky.hpp"

// ---------------------------------------------------------------------------
// Kod generatoru: M-krat vylosuje datum a zapise ho do roury jako "d.m\n".
// ---------------------------------------------------------------------------
static void generuj( int fd_out, int pocet, int za_sekundu )
{
    srand( (unsigned) time( NULL ) ^ (unsigned) getpid() );

    for ( int i = 0; i < pocet; i++ )
    {
        int mesic = rand() % 12;                                // 0..11
        int den   = rand() % g_dni_v_mesici[ mesic ] + 1;       // 1..pocet dni

        char radek[ 16 ];
        int  delka = snprintf( radek, sizeof( radek ), "%d.%d\n", den, mesic + 1 );

        if ( write( fd_out, radek, (size_t) delka ) != delka )
        {
            perror( "generator: write" );
            return;
        }

        usleep( 1000000 / za_sekundu );
    }
}

// ---------------------------------------------------------------------------
// Kod prijemce: cte radky "d.m\n" z roury, doplni jmeno a vypise je na stdout.
// fgets() vrati NULL, az bude roura prazdna a vsechny zapisovaci konce zavrene.
// ---------------------------------------------------------------------------
static void vypis_svatky( int fd_in )
{
    FILE *in = fdopen( fd_in, "r" );

    if ( in == NULL )
    {
        perror( "fdopen" );
        return;
    }

    char radek[ 64 ];

    while ( fgets( radek, sizeof( radek ), in ) != NULL )
    {
        radek[ strcspn( radek, "\n" ) ] = 0;            // odrizne '\n'

        char datum[ sizeof( radek ) + 1 ];             // misto pro tecku navic
        snprintf( datum, sizeof( datum ), "%s.", radek ); // "24.12" -> "24.12."

        printf( "%s %s\n", datum, najdi_svatek( datum ) );
        fflush( stdout );
    }

    fclose( in );   // zavre i deskriptor fd_in
}

// ---------------------------------------------------------------------------
// Vypise na stderr, jak potomek skoncil: normalne (exit) nebo signalem.
// ---------------------------------------------------------------------------
static void hlas_konec( const char *kdo, pid_t pid, int stav )
{
    if ( WIFEXITED( stav ) )
        fprintf( stderr, "rodic: %s (%d) skoncil se stavem %d\n",
                 kdo, (int) pid, WEXITSTATUS( stav ) );
    else if ( WIFSIGNALED( stav ) )
        fprintf( stderr, "rodic: %s (%d) ukoncen signalem %d (%s)\n",
                 kdo, (int) pid, WTERMSIG( stav ), strsignal( WTERMSIG( stav ) ) );
}

int main( int argc, char **argv )
{
    if ( argc != 3 )
    {
        fprintf( stderr, "Pouziti: %s M N\n", argv[ 0 ] );
        fprintf( stderr, "   M   pocet datumu\n" );
        fprintf( stderr, "   N   pocet datumu za sekundu\n" );
        return 1;
    }

    int pocet      = atoi( argv[ 1 ] );
    int za_sekundu = atoi( argv[ 2 ] );

    if ( pocet < 0 || za_sekundu <= 0 )
    {
        fprintf( stderr, "Chyba: M musi byt >= 0 a N > 0.\n" );
        return 1;
    }

    int roura[ 2 ];                 // [0] = cteni, [1] = zapis

    if ( pipe( roura ) < 0 )
    {
        perror( "pipe" );
        return 2;
    }

    // ===================== POTOMEK 1: generator ============================
    pid_t p1 = fork();

    if ( p1 == 0 )
    {
        close( roura[ 0 ] );        // jen zapisuje

        generuj( roura[ 1 ], pocet, za_sekundu );

        close( roura[ 1 ] );
        exit( 0 );
    }

    // ===================== POTOMEK 2: doplni jmena a vypise ================
    pid_t p2 = fork();

    if ( p2 == 0 )
    {
        close( roura[ 1 ] );        // jen cte - jinak by nikdy nedostal EOF

        vypis_svatky( roura[ 0 ] );

        exit( 0 );
    }

    // ===================== RODIC: jen uklidi a ceka ========================
    // Rodic rouru nepouziva, zavre tedy oba konce. Kdyby si nechal otevreny
    // zapisovaci konec, potomek 2 by nikdy nedostal konec roury a necekal
    // by se nikdy konce.
    close( roura[ 0 ] );
    close( roura[ 1 ] );

    if ( p1 < 0 || p2 < 0 )
        perror( "fork" );

    int stav;

    if ( p1 > 0 && waitpid( p1, &stav, 0 ) > 0 )
        hlas_konec( "potomek 1", p1, stav );

    if ( p2 > 0 && waitpid( p2, &stav, 0 ) > 0 )
        hlas_konec( "potomek 2", p2, stav );

    return 0;
}
