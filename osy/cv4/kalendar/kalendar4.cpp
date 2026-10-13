// ============================================================================
// kalendar4.cpp - zadani: pet potomku, ctyri roury
//
// Pouziti:  ./kalendar4 M N
//
//     M ... kolik datumu (a zaroven kolik jmen) se vygeneruje
//     N ... kolik za sekundu (tempo generovani, usleep)
//
// Rozsireni kalendar3 o dve roury (C, D) a dva potomky (4, 5):
//
//                          +-------+
//                          | RODIC |  jen vytvori roury a potomky,
//                          +---+---+  zavre roury a ceka
//          +---------+---------+---------+---------+
//          v         v         v         v         v
//        +----+    +----+    +----+    +----+    +----+
//        | P1 |    | P2 |    | P3 |    | P4 |    | P5 |
//        +----+    +----+    +----+    +----+    +----+
//
//   P1 --A--> P2 --B--> P3 --> stdout     (datum -> doplnit jmeno -> vypis)
//   P1 --C--> P4 --D--> P5 --> stdout     (jmeno -> doplnit datum -> vypis)
//
//   potomek 1 ... generuje nahodna data "den.mesic\n"         -> roura A
//                 a nahodne vybrana jmena "Jmeno\n"           -> roura C
//   potomek 2 ... roura A -> doplni jmeno "den.mesic. Jmeno\n" -> roura B
//   potomek 3 ... roura B -> cislo radku a vypis              -> stdout
//   potomek 4 ... roura C -> doplni datum "den.mesic. Jmeno\n" -> roura D
//   potomek 5 ... roura D -> cislo radku a vypis              -> stdout
//
// Potomek 3 a potomek 5 delaji totez, jen nad jinou rourou - oba volaji
// stejnou funkci vypis_s_cisly(). Potomek 4 posila radky ve stejnem tvaru
// jako potomek 2, proto muze byt kod obou vypisujicich potomku spolecny.
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

// Indexy rour v poli roury[][ 2 ]. Kazda roura ma [0] = cteni, [1] = zapis.
enum { RA, RB, RC, RD, POCET_ROUR };

static int roury[ POCET_ROUR ][ 2 ];

// ---------------------------------------------------------------------------
// Zavre vsechny konce vsech rour KROME dvou zadanych deskriptoru
// (-1 = nic). Kazdy proces ji zavola hned po fork() - "kazdy zavira,
// co nepotrebuje".
// ---------------------------------------------------------------------------
static void zavri_roury_krome( int ponechat1, int ponechat2 )
{
    for ( int r = 0; r < POCET_ROUR; r++ )
        for ( int k = 0; k < 2; k++ )
            if ( roury[ r ][ k ] != ponechat1 && roury[ r ][ k ] != ponechat2 )
                close( roury[ r ][ k ] );
}

// ---------------------------------------------------------------------------
// Zapise cely retezec do deskriptoru. Vraci 0 pri uspechu.
// ---------------------------------------------------------------------------
static int zapis( int fd, const char *text, int delka )
{
    return write( fd, text, (size_t) delka ) == delka ? 0 : -1;
}

// ---------------------------------------------------------------------------
// Najde datum ke jmenu - opak najdi_svatek(). Kdyz jmeno v seznamu neni,
// vrati "?.?.". Jmeno "Statni svatek" je v seznamu dvakrat (28.10. a 17.11.),
// vrati se prvni nalezene datum - i to je pro toto jmeno spravne.
// ---------------------------------------------------------------------------
static const char *najdi_datum( const char *jmeno )
{
    for ( int i = 0; g_svatky[ i ][ 0 ] != nullptr; i++ )
        if ( strcmp( g_svatky[ i ][ 1 ], jmeno ) == 0 )
            return g_svatky[ i ][ 0 ];

    return "?.?.";
}

// ---------------------------------------------------------------------------
// POTOMEK 1: M-krat vylosuje datum (-> roura A) a jmeno (-> roura C).
// ---------------------------------------------------------------------------
static void generuj( int fd_datum, int fd_jmeno, int pocet, int za_sekundu )
{
    srand( (unsigned) time( NULL ) ^ (unsigned) getpid() );

    int pocet_jmen = 0;                         // velikost seznamu svatku
    while ( g_svatky[ pocet_jmen ][ 0 ] != nullptr )
        pocet_jmen++;

    for ( int i = 0; i < pocet; i++ )
    {
        // Nahodne datum podle poctu dni v mesici.
        int mesic = rand() % 12;                                // 0..11
        int den   = rand() % g_dni_v_mesici[ mesic ] + 1;       // 1..pocet dni

        char radek[ 64 ];
        int  delka = snprintf( radek, sizeof( radek ), "%d.%d\n", den, mesic + 1 );

        if ( zapis( fd_datum, radek, delka ) != 0 )
        {
            perror( "potomek 1: write A" );
            return;
        }

        // Nahodne vybrane jmeno ze seznamu svatku.
        const char *jmeno = g_svatky[ rand() % pocet_jmen ][ 1 ];
        delka = snprintf( radek, sizeof( radek ), "%s\n", jmeno );

        if ( zapis( fd_jmeno, radek, delka ) != 0 )
        {
            perror( "potomek 1: write C" );
            return;
        }

        usleep( 1000000 / za_sekundu );
    }
}

// ---------------------------------------------------------------------------
// POTOMEK 2: cte "d.m\n" z roury A, doplni jmeno a posle rourou B
// ve tvaru "den.mesic. Jmeno\n".
// ---------------------------------------------------------------------------
static void doplnit_jmeno( int fd_in, int fd_out )
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
            perror( "potomek 2: write B" );
            break;
        }
    }

    fclose( in );   // zavre i deskriptor fd_in
}

// ---------------------------------------------------------------------------
// POTOMEK 4: cte "Jmeno\n" z roury C, doplni datum a posle rourou D
// ve STEJNEM tvaru jako potomek 2: "den.mesic. Jmeno\n".
// ---------------------------------------------------------------------------
static void doplnit_datum( int fd_in, int fd_out )
{
    FILE *in = fdopen( fd_in, "r" );

    if ( in == NULL )
    {
        perror( "potomek 4: fdopen" );
        return;
    }

    char jmeno[ 64 ];

    while ( fgets( jmeno, sizeof( jmeno ), in ) != NULL )
    {
        jmeno[ strcspn( jmeno, "\n" ) ] = 0;            // odrizne '\n'

        char vystup[ 128 ];
        int  delka = snprintf( vystup, sizeof( vystup ), "%s %s\n",
                               najdi_datum( jmeno ), jmeno );

        if ( zapis( fd_out, vystup, delka ) != 0 )
        {
            perror( "potomek 4: write D" );
            break;
        }
    }

    fclose( in );
}

// ---------------------------------------------------------------------------
// POTOMEK 3 i POTOMEK 5 - spolecny kod.
// Cte hotove radky "den.mesic. Jmeno\n" z roury, pred kazdy da cislo radku
// a vypise. 'kdo' jen rozlisi ve vypisu, ktery potomek radek vypsal.
// ---------------------------------------------------------------------------
static void vypis_s_cisly( int fd_in, const char *kdo )
{
    FILE *in = fdopen( fd_in, "r" );

    if ( in == NULL )
    {
        perror( "vypis: fdopen" );
        return;
    }

    char radek[ 128 ];
    int  cislo = 0;

    while ( fgets( radek, sizeof( radek ), in ) != NULL )
    {
        printf( "%s (%d) %s", kdo, ++cislo, radek );    // radek uz ma '\n'
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
        fprintf( stderr, "   M   pocet datumu a jmen\n" );
        fprintf( stderr, "   N   pocet za sekundu\n" );
        return 1;
    }

    int pocet      = atoi( argv[ 1 ] );
    int za_sekundu = atoi( argv[ 2 ] );

    if ( pocet < 0 || za_sekundu <= 0 )
    {
        fprintf( stderr, "Chyba: M musi byt >= 0 a N > 0.\n" );
        return 1;
    }

    for ( int r = 0; r < POCET_ROUR; r++ )
    {
        if ( pipe( roury[ r ] ) < 0 )
        {
            perror( "pipe" );
            return 2;
        }
    }

    // ===================== POTOMEK 1: generator dat a jmen =================
    pid_t p1 = fork();

    if ( p1 == 0 )
    {
        zavri_roury_krome( roury[ RA ][ 1 ], roury[ RC ][ 1 ] );

        generuj( roury[ RA ][ 1 ], roury[ RC ][ 1 ], pocet, za_sekundu );

        close( roury[ RA ][ 1 ] );  // potomek 2 tim dostane konec roury A
        close( roury[ RC ][ 1 ] );  // potomek 4 tim dostane konec roury C
        exit( 0 );
    }

    // ===================== POTOMEK 2: datum -> doplni jmeno ================
    pid_t p2 = fork();

    if ( p2 == 0 )
    {
        zavri_roury_krome( roury[ RA ][ 0 ], roury[ RB ][ 1 ] );

        doplnit_jmeno( roury[ RA ][ 0 ], roury[ RB ][ 1 ] );

        close( roury[ RB ][ 1 ] );
        exit( 0 );
    }

    // ===================== POTOMEK 3: vypis z roury B ======================
    pid_t p3 = fork();

    if ( p3 == 0 )
    {
        zavri_roury_krome( roury[ RB ][ 0 ], -1 );

        vypis_s_cisly( roury[ RB ][ 0 ], "P3" );

        exit( 0 );
    }

    // ===================== POTOMEK 4: jmeno -> doplni datum ================
    pid_t p4 = fork();

    if ( p4 == 0 )
    {
        zavri_roury_krome( roury[ RC ][ 0 ], roury[ RD ][ 1 ] );

        doplnit_datum( roury[ RC ][ 0 ], roury[ RD ][ 1 ] );

        close( roury[ RD ][ 1 ] );
        exit( 0 );
    }

    // ===================== POTOMEK 5: vypis z roury D ======================
    pid_t p5 = fork();

    if ( p5 == 0 )
    {
        zavri_roury_krome( roury[ RD ][ 0 ], -1 );

        vypis_s_cisly( roury[ RD ][ 0 ], "P5" );    // stejna funkce jako P3

        exit( 0 );
    }

    // ===================== RODIC: uklidi a ceka ============================
    // Rodic zadnou rouru nepouziva - zavre vsech osm koncu.
    zavri_roury_krome( -1, -1 );

    pid_t       potomci[ 5 ] = { p1, p2, p3, p4, p5 };
    const char *jmena[ 5 ]   = { "potomek 1", "potomek 2", "potomek 3",
                                 "potomek 4", "potomek 5" };

    for ( int i = 0; i < 5; i++ )
    {
        if ( potomci[ i ] < 0 )
        {
            fprintf( stderr, "rodic: %s se nepodarilo vytvorit\n", jmena[ i ] );
            continue;
        }

        int stav;

        if ( waitpid( potomci[ i ], &stav, 0 ) > 0 )
            hlas_konec( jmena[ i ], potomci[ i ], stav );
    }

    return 0;
}
