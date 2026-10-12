// ============================================================================
// kolona1.cpp - 1. krok: tri potomci, dve roury, tri prikazy pres exec
//
// Pouziti:  ./kolona1 soubor
//
// Program dela totez jako slozeny prikaz shellu:
//
//     sort < soubor | nl -s ". " | tr a-z A-Z
//
//   soubor --> [ POTOMEK 1: sort ] --roura 1--> [ POTOMEK 2: nl ] --roura 2--> [ POTOMEK 3: tr ] --> stdout
//
// Kazdy potomek si nejdriv pomoci dup2() presmeruje stdin/stdout do souboru
// nebo do roury, zavre vsechno, co nepotrebuje, a pak se pomoci execlp()
// promeni v zadany program. Ten pak cte svuj stdin a pise na svuj stdout -
// vubec nevi, ze jsou to roury.
//
// Test:     sort < names.txt | nl -s ". " | tr a-z A-Z > out1
//           ./kolona1 names.txt > out2
//           diff out1 out2               (nesmi nic vypsat)
//
// Predmet: Operacni systemy
// ============================================================================

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>       // fork, pipe, dup2, close, execlp
#include <fcntl.h>        // open
#include <sys/wait.h>     // waitpid

int main( int argc, char **argv )
{
    if ( argc != 2 )
    {
        fprintf( stderr, "Pouziti: %s soubor\n", argv[ 0 ] );
        return 1;
    }

    int roura1[ 2 ];                // sort -> nl
    int roura2[ 2 ];                // nl   -> tr

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

        close( x );                 // puvodni deskriptory uz nejsou potreba
        close( roura1[ 0 ] );
        close( roura1[ 1 ] );
        close( roura2[ 0 ] );
        close( roura2[ 1 ] );

        execlp( "sort", "sort", NULL );
        perror( "sort" );           // sem se dojde jen kdyz exec selze
        exit( 1 );
    }

    // ===================== POTOMEK 2: nl -s ". " ===========================
    if ( fork() == 0 )
    {
        dup2( roura1[ 0 ], 0 );     // stdin  = roura 1
        dup2( roura2[ 1 ], 1 );     // stdout = roura 2

        close( roura1[ 0 ] );
        close( roura1[ 1 ] );
        close( roura2[ 0 ] );
        close( roura2[ 1 ] );

        execlp( "nl", "nl", "-s", ". ", NULL );
        perror( "nl" );
        exit( 1 );
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
    // Kdyby rodic nechal otevreny zapisovaci konec roury, nl ani tr by
    // nikdy nedostaly konec vstupu a program by se nikdy neukoncil.
    close( roura1[ 0 ] );
    close( roura1[ 1 ] );
    close( roura2[ 0 ] );
    close( roura2[ 1 ] );

    for ( int i = 0; i < 3; i++ )
        wait( NULL );

    return 0;
}
