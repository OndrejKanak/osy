// ============================================================================
// v4_poll.cpp - varianta 4: jeden proces cte ze DVOU rour najednou (poll)
//
// Pouziti:  ./v4_poll M N1 N2
//
//     M  ... kolik datumu vygeneruje kazdy generator
//     N1 ... rychlost generatoru 1 (datumu za sekundu)
//     N2 ... rychlost generatoru 2
//
//      +-------------+   ROURA A
//      | GENERATOR 1 |-------------+
//      +-------------+             v
//                             +----------+
//                             | PRIJEMCE | ---> stdout
//                             +----------+
//      +-------------+             ^
//      | GENERATOR 2 |-------------+
//      +-------------+   ROURA B
//
// Proc poll(): kdyby prijemce cetl nejdriv z A pomoci read(), zablokoval by
// se tam - i kdyz v B uz cekaji data. poll() ceka na VICE deskriptoru
// najednou a vrati se, jakmile je ktery z nich pripraveny ke cteni.
//
// Proc tady neni fgets(): stdio si data nacita do sveho bufferu "do zasoby".
// poll() o tom bufferu nevi - hlasi jen data, ktera jsou jeste v roure.
// Proto se cte primo read() a radky se skladaji rucne.
//
// Predmet: Operacni systemy
// ============================================================================

#include <time.h>
#include <poll.h>
#include <sys/wait.h>

#include "spolecne.hpp"

// ---------------------------------------------------------------------------
// Generator: "den.mesic\n" do roury zadanou rychlosti.
// ---------------------------------------------------------------------------
static void generuj( int fd_out, int pocet, int za_sekundu )
{
    srand( (unsigned) time( NULL ) ^ (unsigned) getpid() );

    for ( int i = 0; i < pocet; i++ )
    {
        char datum[ 16 ], radek[ 20 ];

        nahodne_datum( datum, sizeof( datum ) );
        snprintf( radek, sizeof( radek ), "%s\n", datum );

        if ( zapis_vse( fd_out, radek ) != 0 )
            return;

        usleep( 1000000 / za_sekundu );
    }
}

// ---------------------------------------------------------------------------
// Rozpracovany radek jedne roury. read() muze vratit kus radku nebo vic
// radku najednou - neuplny konec si schovame do dalsiho cteni.
// ---------------------------------------------------------------------------
struct vstup
{
    const char *jmeno;
    char        buffer[ 256 ];
    size_t      delka;
    int         prijato;
};

// Vypise vsechny cele radky, ktere jsou v bufferu.
static void zpracuj_radky( struct vstup *v )
{
    char *zacatek = v->buffer;
    char *konec;

    while ( ( konec = (char *) memchr( zacatek, '\n',
                                       v->delka - (size_t) ( zacatek - v->buffer ) ) ) != NULL )
    {
        *konec = 0;

        char vystup[ 128 ];
        doplnit_jmeno( zacatek, vystup, sizeof( vystup ) );
        printf( "[%s] %s\n", v->jmeno, vystup );
        fflush( stdout );

        v->prijato++;
        zacatek = konec + 1;
    }

    // Neuplny zbytek posuneme na zacatek bufferu.
    v->delka -= (size_t) ( zacatek - v->buffer );
    memmove( v->buffer, zacatek, v->delka );
}

int main( int argc, char **argv )
{
    if ( argc != 4 )
    {
        fprintf( stderr, "Pouziti: %s M N1 N2\n", argv[ 0 ] );
        fprintf( stderr, "Priklad: %s 10 4 1   (A je 4x rychlejsi nez B)\n", argv[ 0 ] );
        return 1;
    }

    int pocet = nacti_cislo( argv[ 1 ], 0, "M" );
    int n1    = nacti_cislo( argv[ 2 ], 1, "N1" );
    int n2    = nacti_cislo( argv[ 3 ], 1, "N2" );

    int roura_a[ 2 ];
    int roura_b[ 2 ];

    if ( pipe( roura_a ) < 0 || pipe( roura_b ) < 0 )
    {
        perror( "pipe" );
        return 2;
    }

    // ===================== POTOMEK 1: generator A ==========================
    pid_t g1 = fork();

    if ( g1 == 0 )
    {
        close( roura_a[ 0 ] );
        close( roura_b[ 0 ] );
        close( roura_b[ 1 ] );

        generuj( roura_a[ 1 ], pocet, n1 );

        close( roura_a[ 1 ] );
        exit( 0 );
    }

    // ===================== POTOMEK 2: generator B ==========================
    pid_t g2 = fork();

    if ( g2 == 0 )
    {
        close( roura_a[ 0 ] );
        close( roura_a[ 1 ] );
        close( roura_b[ 0 ] );

        generuj( roura_b[ 1 ], pocet, n2 );

        close( roura_b[ 1 ] );
        exit( 0 );
    }

    // ===================== POTOMEK 3: prijemce s poll() ====================
    pid_t p3 = fork();

    if ( p3 == 0 )
    {
        close( roura_a[ 1 ] );
        close( roura_b[ 1 ] );

        struct vstup  vstupy[ 2 ] = { { "A", {}, 0, 0 }, { "B", {}, 0, 0 } };
        struct pollfd sledovane[ 2 ];

        sledovane[ 0 ].fd     = roura_a[ 0 ];
        sledovane[ 0 ].events = POLLIN;
        sledovane[ 1 ].fd     = roura_b[ 0 ];
        sledovane[ 1 ].events = POLLIN;

        int otevrenych = 2;

        while ( otevrenych > 0 )
        {
            // Ceka, dokud nektera roura nema data nebo neskoncila.
            // -1 = cekat bez casoveho limitu.
            if ( poll( sledovane, 2, -1 ) < 0 )
            {
                if ( errno == EINTR )
                    continue;

                perror( "poll" );
                break;
            }

            for ( int i = 0; i < 2; i++ )
            {
                // POLLIN = jsou data, POLLHUP = zapisovatel zavrel rouru.
                if ( !( sledovane[ i ].revents & ( POLLIN | POLLHUP ) ) )
                    continue;

                struct vstup *v = &vstupy[ i ];

                if ( v->delka >= sizeof( v->buffer ) - 1 )   // radek bez '\n' - zahodit
                    v->delka = 0;
                ssize_t       n = read( sledovane[ i ].fd, v->buffer + v->delka,
                                        sizeof( v->buffer ) - 1 - v->delka );

                if ( n > 0 )
                {
                    v->delka += (size_t) n;
                    zpracuj_radky( v );
                }
                else if ( n == 0 )
                {
                    // Konec roury: zavreme ji a poll() ji dal nesleduje
                    // (zaporny fd poll() preskakuje).
                    fprintf( stderr, "prijemce: roura %s skoncila\n", v->jmeno );
                    close( sledovane[ i ].fd );
                    sledovane[ i ].fd = -1;
                    otevrenych--;
                }
            }
        }

        fprintf( stderr, "prijemce: z A prijato %d, z B prijato %d datumu\n",
                 vstupy[ 0 ].prijato, vstupy[ 1 ].prijato );
        exit( 0 );
    }

    // ===================== RODIC ===========================================
    close( roura_a[ 0 ] );
    close( roura_a[ 1 ] );
    close( roura_b[ 0 ] );
    close( roura_b[ 1 ] );

    pid_t potomci[ 3 ] = { g1, g2, p3 };

    for ( int i = 0; i < 3; i++ )
        if ( potomci[ i ] > 0 )
            waitpid( potomci[ i ], NULL, 0 );

    fprintf( stderr, "rodic: vsichni potomci skoncili\n" );

    return 0;
}
