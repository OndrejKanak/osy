// ============================================================================
// v3_fifo_cti.cpp - varianta 3: ctenar POJMENOVANE roury
//
// Pouziti:  ./v3_fifo_cti FIFO
//
// Otevre pojmenovanou rouru pro cteni, kazde datum doplni o jmeno a cislo
// radku a vypise. Skonci, az se odpoji vsichni zapisovatele (konec roury).
//
//     terminal 1:   ./v3_fifo_cti kalendar.fifo
//     terminal 2:   ./v3_fifo_gen kalendar.fifo 20 2
//     terminal 3:   ./v3_fifo_gen kalendar.fifo 20 5     (muze i druhy)
//
// Obsah roury se da cist i obycejnymi prikazy:
//     cat kalendar.fifo                        misto ctenare
//     echo 24.12 > kalendar.fifo               misto generatoru
//
// Predmet: Operacni systemy
// ============================================================================

#include <fcntl.h>        // open
#include <sys/stat.h>     // mkfifo

#include "spolecne.hpp"

int main( int argc, char **argv )
{
    if ( argc != 2 )
    {
        fprintf( stderr, "Pouziti: %s FIFO\n", argv[ 0 ] );
        fprintf( stderr, "Priklad: %s kalendar.fifo\n", argv[ 0 ] );
        return 1;
    }

    const char *fifo = argv[ 1 ];

    if ( mkfifo( fifo, 0600 ) < 0 && errno != EEXIST )
    {
        perror( "mkfifo" );
        return 2;
    }

    fprintf( stderr, "ctenar: oteviram %s pro cteni - cekam na generator...\n", fifo );

    // open() pro cteni se ZABLOKUJE, dokud rouru neotevre nekdo pro zapis.
    int fd = open( fifo, O_RDONLY );

    if ( fd < 0 )
    {
        perror( fifo );
        return 2;
    }

    fprintf( stderr, "ctenar: generator pripojen.\n" );

    FILE *in = fdopen( fd, "r" );
    char  radek[ 64 ], vystup[ 128 ];
    int   cislo = 0;

    while ( in != NULL && fgets( radek, sizeof( radek ), in ) != NULL )
    {
        doplnit_jmeno( radek, vystup, sizeof( vystup ) );
        printf( "(%d) %s\n", ++cislo, vystup );
        fflush( stdout );
    }

    if ( in != NULL )
        fclose( in );

    fprintf( stderr, "ctenar: konec roury, prijato %d datumu.\n", cislo );
    fprintf( stderr, "ctenar: soubor %s zustava, smazes ho: rm %s\n", fifo, fifo );

    return 0;
}
