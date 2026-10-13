// ============================================================================
// kalendar3.cpp - 3. verze: tri potomci, dve roury
//
// Pouziti:  ./kalendar3 M N
//
//     M ... kolik datumu se vygeneruje
//     N ... kolik datumu za sekundu (tempo generovani, usleep)
//
//                   +-----------------------+
//                   | HLAVNI RODIC (Parent) |
//                   +-----------+-----------+
//                               |
//                 +-------------+-------------+
//                 |             |             |
//                 v             v             v
//           +-----------+ +-----------+ +-----------+
//           | POTOMEK 1 | | POTOMEK 2 | | POTOMEK 3 |
//           +---+-------+ +---+---+---+ +-------+---+
//               |             ^   |             ^
//               |   ROURA A   |   |   ROURA B   |
//               +->[fd==>fd]--+   +->[fd==>fd]--+
//
//   potomek 1 ... generuje "den.mesic\n"                   -> roura A
//   potomek 2 ... roura A -> doplni jmeno "den.mesic. Jmeno\n" -> roura B
//   potomek 3 ... roura B -> prida cislo radku "(1) den.mesic. Jmeno" -> stdout
//   rodic     ... jen vytvori roury a potomky, zavre roury a ceka
//
// Zakladni pravidlo: za fork() kazdy proces hned zavre vsechny konce rour,
// ktere nepotrebuje. Jinak se konec roury (EOF) nikdy nedostane ke ctenari.
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
// Zapise cely retezec do deskriptoru. Vraci 0 pri uspechu.
// ---------------------------------------------------------------------------
static int zapis( int fd, const char *text, int delka )
{
    return write( fd, text, (size_t) delka ) == delka ? 0 : -1;
}

// ---------------------------------------------------------------------------
// POTOMEK 1: M-krat vylosuje datum a zapise ho do roury A jako "d.m\n".
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

        if ( zapis( fd_out, radek, delka ) != 0 )
        {
            perror( "potomek 1: write" );
            return;
        }

        usleep( 1000000 / za_sekundu );
    }
}

// ---------------------------------------------------------------------------
// POTOMEK 2: cte "d.m\n" z roury A, doplni jmeno a posle dal rourou B.
// Nic nevypisuje.
// ---------------------------------------------------------------------------
static void doplnit_svatky( int fd_in, int fd_out )
{
    FILE *in = fdopen( fd_in, "r" );

    if ( in == NULL )
    {
        perror( "potomek 2: fdopen" );
        return;
    }

    char radek[ 64 ];

    while ( fgets( radek, sizeof( radek ), in ) != NULL )
    {
        radek[ strcspn( radek, "\n" ) ] = 0;            // odrizne '\n'

        char datum[ sizeof( radek ) + 1 ];             // misto pro tecku navic
        snprintf( datum, sizeof( datum ), "%s.", radek ); // "24.12" -> "24.12."

        char vystup[ 128 ];
        int  delka = snprintf( vystup, sizeof( vystup ), "%s %s\n",
                               datum, najdi_svatek( datum ) );

        if ( zapis( fd_out, vystup, delka ) != 0 )
        {
            perror( "potomek 2: write" );
            break;
        }
    }

    fclose( in );   // zavre i deskriptor fd_in
}

// ---------------------------------------------------------------------------
// POTOMEK 3: cte hotove radky z roury B, pred kazdy da cislo radku a vypise.
// ---------------------------------------------------------------------------
static void vypis_s_cisly( int fd_in )
{
    FILE *in = fdopen( fd_in, "r" );

    if ( in == NULL )
    {
        perror( "potomek 3: fdopen" );
        return;
    }

    char radek[ 128 ];
    int  cislo = 0;

    while ( fgets( radek, sizeof( radek ), in ) != NULL )
    {
        printf( "(%d) %s", ++cislo, radek );            // radek uz ma '\n'
        fflush( stdout );
    }

    fclose( in );
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

    int roura_a[ 2 ];               // potomek 1 -> potomek 2
    int roura_b[ 2 ];               // potomek 2 -> potomek 3

    if ( pipe( roura_a ) < 0 || pipe( roura_b ) < 0 )
    {
        perror( "pipe" );
        return 2;
    }

    // ===================== POTOMEK 1: generator ============================
    pid_t p1 = fork();

    if ( p1 == 0 )
    {
        close( roura_a[ 0 ] );      // z A necte
        close( roura_b[ 0 ] );      // B vubec nepouziva
        close( roura_b[ 1 ] );

        generuj( roura_a[ 1 ], pocet, za_sekundu );

        close( roura_a[ 1 ] );      // potomek 2 tim dostane konec roury A
        exit( 0 );
    }

    // ===================== POTOMEK 2: doplni jmena =========================
    pid_t p2 = fork();

    if ( p2 == 0 )
    {
        close( roura_a[ 1 ] );      // do A nezapisuje
        close( roura_b[ 0 ] );      // z B necte

        doplnit_svatky( roura_a[ 0 ], roura_b[ 1 ] );

        close( roura_b[ 1 ] );      // potomek 3 tim dostane konec roury B
        exit( 0 );
    }

    // ===================== POTOMEK 3: cisla radku a vypis ==================
    pid_t p3 = fork();

    if ( p3 == 0 )
    {
        close( roura_a[ 0 ] );      // A vubec nepouziva
        close( roura_a[ 1 ] );
        close( roura_b[ 1 ] );      // do B nezapisuje

        vypis_s_cisly( roura_b[ 0 ] );

        exit( 0 );
    }

    // ===================== RODIC: uklidi a ceka ============================
    // Rodic zadnou rouru nepouziva - zavre vsechny ctyri konce. Kdyby nechal
    // otevreny zapisovaci konec, ctenar by konec roury nikdy nedostal.
    close( roura_a[ 0 ] );
    close( roura_a[ 1 ] );
    close( roura_b[ 0 ] );
    close( roura_b[ 1 ] );

    if ( p1 < 0 || p2 < 0 || p3 < 0 )
        perror( "fork" );

    pid_t potomci[ 3 ] = { p1, p2, p3 };

    const char *jmena[ 3 ]   = { "potomek 1", "potomek 2", "potomek 3" };

    for ( int i = 0; i < 3; i++ )
    {
        int stav;

        if ( potomci[ i ] > 0 && waitpid( potomci[ i ], &stav, 0 ) > 0 )
            hlas_konec( jmena[ i ], potomci[ i ], stav );
    }

    return 0;
}
