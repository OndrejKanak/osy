// ============================================================================
// v2_dva_generatory.cpp - varianta 2: dva generatory zapisuji do jedne roury
//
// Pouziti:  ./v2_dva_generatory M N1 N2
//
//     M  ... kolik datumu vygeneruje KAZDY generator
//     N1 ... rychlost generatoru 1 (datumu za sekundu)
//     N2 ... rychlost generatoru 2
//
//      +-------------+
//      | GENERATOR 1 |--+
//      +-------------+  |   ROURA A    +-----------+
//                       +-----------> | PRIJEMCE  | ---> stdout
//      +-------------+  |              +-----------+
//      | GENERATOR 2 |--+
//      +-------------+
//
// Kazdy generator posila "cislo_generatoru:den.mesic\n". Prijemce doplni
// jmeno a vypise, od koho datum prislo.
//
// Dve dulezite vlastnosti roury:
//   - zapis do 4 kB (PIPE_BUF) je ATOMICKY: radky obou generatoru se mohou
//     stridat, ale nikdy se neprolozi uprostred radku
//   - prijemce dostane konec roury az po zavreni zapisovaciho konce
//     U OBOU generatoru - skonci tedy az po tom pomalejsim
//
// Predmet: Operacni systemy
// ============================================================================

#include <time.h>
#include <sys/wait.h>

#include "spolecne.hpp"

// ---------------------------------------------------------------------------
// Kod generatoru - spolecny pro oba, lisi se jen cislem a rychlosti.
// ---------------------------------------------------------------------------
static void generuj( int fd_out, int cislo, int pocet, int za_sekundu )
{
    srand( (unsigned) time( NULL ) ^ (unsigned) getpid() );

    for ( int i = 0; i < pocet; i++ )
    {
        char datum[ 16 ], radek[ 32 ];

        nahodne_datum( datum, sizeof( datum ) );
        snprintf( radek, sizeof( radek ), "%d:%s\n", cislo, datum );

        // Jeden write() na cely radek - diky tomu je zapis atomicky.
        if ( zapis_vse( fd_out, radek ) != 0 )
            return;

        usleep( 1000000 / za_sekundu );
    }
}

int main( int argc, char **argv )
{
    if ( argc != 4 )
    {
        fprintf( stderr, "Pouziti: %s M N1 N2\n", argv[ 0 ] );
        fprintf( stderr, "   M    pocet datumu od kazdeho generatoru\n" );
        fprintf( stderr, "   N1   rychlost generatoru 1 (za sekundu)\n" );
        fprintf( stderr, "   N2   rychlost generatoru 2 (za sekundu)\n" );
        return 1;
    }

    int pocet = nacti_cislo( argv[ 1 ], 0, "M" );
    int n1    = nacti_cislo( argv[ 2 ], 1, "N1" );
    int n2    = nacti_cislo( argv[ 3 ], 1, "N2" );

    int roura[ 2 ];

    if ( pipe( roura ) < 0 )
    {
        perror( "pipe" );
        return 2;
    }

    // ===================== POTOMEK 1: generator 1 ==========================
    pid_t g1 = fork();

    if ( g1 == 0 )
    {
        close( roura[ 0 ] );
        generuj( roura[ 1 ], 1, pocet, n1 );
        close( roura[ 1 ] );
        exit( 0 );
    }

    // ===================== POTOMEK 2: generator 2 ==========================
    pid_t g2 = fork();

    if ( g2 == 0 )
    {
        close( roura[ 0 ] );
        generuj( roura[ 1 ], 2, pocet, n2 );
        close( roura[ 1 ] );
        exit( 0 );
    }

    // ===================== POTOMEK 3: prijemce =============================
    pid_t p3 = fork();

    if ( p3 == 0 )
    {
        close( roura[ 1 ] );        // nezapisuje - jinak by nikdy nedostal EOF

        FILE *in = fdopen( roura[ 0 ], "r" );
        char  radek[ 64 ], vystup[ 128 ];
        int   od_generatoru[ 3 ] = { 0, 0, 0 };

        while ( in != NULL && fgets( radek, sizeof( radek ), in ) != NULL )
        {
            int   cislo     = atoi( radek );            // cislo pred ':'
            char *dvojtecka = strchr( radek, ':' );

            if ( dvojtecka == NULL || cislo < 1 || cislo > 2 )
                continue;

            doplnit_jmeno( dvojtecka + 1, vystup, sizeof( vystup ) );
            od_generatoru[ cislo ]++;

            printf( "[generator %d] %s\n", cislo, vystup );
            fflush( stdout );
        }

        fprintf( stderr, "prijemce: od generatoru 1 prislo %d, od generatoru 2 %d datumu\n",
                 od_generatoru[ 1 ], od_generatoru[ 2 ] );
        exit( 0 );
    }

    // ===================== RODIC ===========================================
    close( roura[ 0 ] );
    close( roura[ 1 ] );

    pid_t       potomci[ 3 ] = { g1, g2, p3 };
    const char *jmena[ 3 ]   = { "generator 1", "generator 2", "prijemce" };

    for ( int i = 0; i < 3; i++ )
    {
        int stav;

        if ( potomci[ i ] > 0 && waitpid( potomci[ i ], &stav, 0 ) > 0 )
            fprintf( stderr, "rodic: %s (%d) skoncil\n", jmena[ i ], (int) potomci[ i ] );
    }

    return 0;
}
