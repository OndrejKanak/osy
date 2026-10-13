// ============================================================================
// v3_rozvetveni.cpp - varianta 3: jeden zdroj, dve vetve
//
// Pouziti:  ./v3_rozvetveni soubor vystup_a vystup_b
//
//                                   +-- roura A --> [ P2: sort    ] --> vystup_a
//   soubor --> [ P1: rozdvoji ] ----+
//                                   +-- roura B --> [ P3: sort -r ] --> vystup_b
//
// Potomek 1 cte soubor a vse, co precte, zapise do OBOU rour (vlastni tee).
// Potomek 2 a 3 delaji totez, jen s jinym smerem trideni a do jineho
// souboru - proto je jejich kod jedna funkce setrid().
//
// Test:     ./v3_rozvetveni names.txt a.txt b.txt
//           sort    < names.txt | diff - a.txt
//           sort -r < names.txt | diff - b.txt
//
// Predmet: Operacni systemy
// ============================================================================

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>       // fork, pipe, dup2, close, execlp, read, write
#include <fcntl.h>        // open
#include <sys/wait.h>     // wait

static int roura_a[ 2 ];
static int roura_b[ 2 ];

static void zavri_roury( void )
{
    close( roura_a[ 0 ] );
    close( roura_a[ 1 ] );
    close( roura_b[ 0 ] );
    close( roura_b[ 1 ] );
}

// ---------------------------------------------------------------------------
// Zapise cely blok - write() muze do roury zapsat i mene, nez se chtelo.
// ---------------------------------------------------------------------------
static int zapis_vse( int fd, const char *data, ssize_t delka )
{
    while ( delka > 0 )
    {
        ssize_t n = write( fd, data, (size_t) delka );

        if ( n < 0 )
            return -1;

        data  += n;
        delka -= n;
    }

    return 0;
}

// ---------------------------------------------------------------------------
// POTOMEK 1: cte stdin a kazdy precteny blok posle do obou rour.
// Na hranicich radku nezalezi - bajty jdou v poradi, v jakem prisly.
// ---------------------------------------------------------------------------
static void rozdvoj( int fd_a, int fd_b )
{
    char    buffer[ 4096 ];
    ssize_t n;

    while ( ( n = read( 0, buffer, sizeof( buffer ) ) ) > 0 )
    {
        if ( zapis_vse( fd_a, buffer, n ) != 0 || zapis_vse( fd_b, buffer, n ) != 0 )
        {
            perror( "potomek 1: write" );
            return;
        }
    }
}

// ---------------------------------------------------------------------------
// POTOMEK 2 i 3 - spolecny kod: stdin z roury, stdout do souboru, sort.
// ---------------------------------------------------------------------------
static void setrid( int fd_in, const char *vystup, int obracene )
{
    int y = open( vystup, O_WRONLY | O_CREAT | O_TRUNC, 0644 );

    if ( y < 0 )
    {
        perror( vystup );
        exit( 1 );
    }

    dup2( fd_in, 0 );
    dup2( y, 1 );
    close( y );
    zavri_roury();

    if ( obracene )
        execlp( "sort", "sort", "-r", NULL );
    else
        execlp( "sort", "sort", NULL );

    perror( "sort" );
    exit( 127 );
}

int main( int argc, char **argv )
{
    if ( argc != 4 )
    {
        fprintf( stderr, "Pouziti: %s soubor vystup_a vystup_b\n", argv[ 0 ] );
        return 1;
    }

    if ( pipe( roura_a ) < 0 || pipe( roura_b ) < 0 )
    {
        perror( "pipe" );
        return 2;
    }

    // ===================== POTOMEK 1: cte soubor, posila do A i B ==========
    if ( fork() == 0 )
    {
        int x = open( argv[ 1 ], O_RDONLY );

        if ( x < 0 )
        {
            perror( argv[ 1 ] );
            exit( 1 );
        }

        dup2( x, 0 );
        close( x );
        close( roura_a[ 0 ] );      // z rour necte
        close( roura_b[ 0 ] );

        rozdvoj( roura_a[ 1 ], roura_b[ 1 ] );

        close( roura_a[ 1 ] );      // obe vetve tim dostanou konec roury
        close( roura_b[ 1 ] );
        exit( 0 );
    }

    // ===================== POTOMEK 2: vetev A - sort =======================
    if ( fork() == 0 )
        setrid( roura_a[ 0 ], argv[ 2 ], 0 );

    // ===================== POTOMEK 3: vetev B - sort -r ====================
    if ( fork() == 0 )
        setrid( roura_b[ 0 ], argv[ 3 ], 1 );

    // ===================== RODIC ===========================================
    zavri_roury();

    for ( int i = 0; i < 3; i++ )
        wait( NULL );

    return 0;
}
