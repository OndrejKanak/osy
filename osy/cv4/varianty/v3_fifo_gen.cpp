// ============================================================================
// v3_fifo_gen.cpp - varianta 3: generator zapisuje do POJMENOVANE roury
//
// Pouziti:  ./v3_fifo_gen FIFO M N
//
//     FIFO ... jmeno pojmenovane roury, napr. kalendar.fifo
//     M    ... kolik datumu se vygeneruje
//     N    ... kolik datumu za sekundu
//
// Do roury zapisuje "den.mesic\n". Cte je druhy program v3_fifo_cti,
// spusteny v JINEM terminalu:
//
//     terminal 1:   ./v3_fifo_cti kalendar.fifo
//     terminal 2:   ./v3_fifo_gen kalendar.fifo 20 2
//
// Rozdil proti pipe(): obycejna roura existuje jen mezi rodicem a potomky,
// kteri ji zdedi pres fork(). Pojmenovana roura (FIFO) je soubor
// v adresari - otevrit ji muze libovolny proces, ktery zna jeji jmeno.
// Data ale na disk nejdou, jadro je predava v pameti stejne jako u pipe().
//
// Predmet: Operacni systemy
// ============================================================================

#include <time.h>
#include <fcntl.h>        // open
#include <sys/stat.h>     // mkfifo

#include "spolecne.hpp"

int main( int argc, char **argv )
{
    if ( argc != 4 )
    {
        fprintf( stderr, "Pouziti: %s FIFO M N\n", argv[ 0 ] );
        fprintf( stderr, "Priklad: %s kalendar.fifo 20 2\n", argv[ 0 ] );
        return 1;
    }

    const char *fifo       = argv[ 1 ];
    int         pocet      = nacti_cislo( argv[ 2 ], 0, "M" );
    int         za_sekundu = nacti_cislo( argv[ 3 ], 1, "N" );

    // Rouru vytvori ten, kdo prijde prvni - druhemu mkfifo() vrati EEXIST.
    if ( mkfifo( fifo, 0600 ) < 0 && errno != EEXIST )
    {
        perror( "mkfifo" );
        return 2;
    }

    fprintf( stderr, "generator: oteviram %s pro zapis - cekam na ctenare...\n", fifo );

    // open() pro zapis se ZABLOKUJE, dokud rouru neotevre nekdo pro cteni.
    int fd = open( fifo, O_WRONLY );

    if ( fd < 0 )
    {
        perror( fifo );
        return 2;
    }

    fprintf( stderr, "generator: ctenar pripojen, posilam %d datumu.\n", pocet );

    srand( (unsigned) time( NULL ) ^ (unsigned) getpid() );

    for ( int i = 0; i < pocet; i++ )
    {
        char datum[ 16 ], radek[ 20 ];

        nahodne_datum( datum, sizeof( datum ) );
        snprintf( radek, sizeof( radek ), "%s\n", datum );

        // Kdyz ctenar mezitim skonci, write() vyvola SIGPIPE a program
        // skonci - stejne jako u obycejne roury.
        if ( zapis_vse( fd, radek ) != 0 )
        {
            perror( "write" );
            break;
        }

        fprintf( stderr, "generator: odeslano %s", radek );
        usleep( 1000000 / za_sekundu );
    }

    close( fd );        // ctenar tim dostane konec roury
    fprintf( stderr, "generator: hotovo.\n" );

    return 0;
}
