// ============================================================================
// v4_parametry.cpp - varianta 4: vic parametru a druhu presmerovani
//
// Pouziti:  ./v4_parametry [-a] [-s oddelovac] [-e chyby] [vstup [vystup]]
//
//     vstup          ... soubor se jmeny; bez nej (nebo "-") se cte stdin
//     vystup         ... kam jde vysledek; bez nej na obrazovku
//     -a             ... vystup PRIPISOVAT na konec souboru     (jako >>)
//     -s oddelovac   ... oddelovac pro nl, vychozi ". "
//     -e chyby       ... stderr vsech potomku do souboru         (jako 2>)
//
// Odpovida prikazu shellu:
//
//     sort < vstup | nl -s ". " | tr a-z A-Z > vystup
//
// Priklady:
//     ./v4_parametry names.txt                     jako kolona1
//     ./v4_parametry < names.txt                   vstup ze stdin
//     cat names.txt | ./v4_parametry               taky stdin
//     ./v4_parametry names.txt out                 vystup do souboru (prepise)
//     ./v4_parametry -a names.txt out              pripise na konec
//     ./v4_parametry -s ": " names.txt             jiny oddelovac
//     ./v4_parametry -e chyby.txt neni.txt         chybova hlaska do chyby.txt
//
// Predmet: Operacni systemy
// ============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>       // fork, pipe, dup2, close, execlp, getopt
#include <fcntl.h>        // open
#include <sys/wait.h>     // wait

static int roura1[ 2 ];
static int roura2[ 2 ];

static void zavri_roury( void )
{
    close( roura1[ 0 ] );
    close( roura1[ 1 ] );
    close( roura2[ 0 ] );
    close( roura2[ 1 ] );
}

// ---------------------------------------------------------------------------
// Otevre soubor a presmeruje do nej deskriptor 'kam' (0, 1 nebo 2).
// Pri chybe ukonci potomka.
// ---------------------------------------------------------------------------
static void presmeruj( const char *soubor, int priznaky, int kam )
{
    int fd = open( soubor, priznaky, 0644 );

    if ( fd < 0 )
    {
        perror( soubor );
        exit( 1 );
    }

    dup2( fd, kam );
    close( fd );
}

int main( int argc, char **argv )
{
    int         pripsat    = 0;
    const char *oddelovac  = ". ";
    const char *chyby      = NULL;
    int         c;

    while ( ( c = getopt( argc, argv, "as:e:" ) ) != -1 )
    {
        switch ( c )
        {
            case 'a': pripsat   = 1;      break;
            case 's': oddelovac = optarg; break;
            case 'e': chyby     = optarg; break;
            default:
                fprintf( stderr, "Pouziti: %s [-a] [-s oddelovac] [-e chyby] [vstup [vystup]]\n",
                         argv[ 0 ] );
                return 1;
        }
    }

    const char *vstup  = ( optind < argc )     ? argv[ optind ]     : NULL;
    const char *vystup = ( optind + 1 < argc ) ? argv[ optind + 1 ] : NULL;

    if ( vstup != NULL && strcmp( vstup, "-" ) == 0 )
        vstup = NULL;

    // O_TRUNC = prepsat (>),  O_APPEND = pripsat na konec (>>)
    int priznaky_vystupu = O_WRONLY | O_CREAT | ( pripsat ? O_APPEND : O_TRUNC );

    if ( pipe( roura1 ) < 0 || pipe( roura2 ) < 0 )
    {
        perror( "pipe" );
        return 2;
    }

    // ===================== POTOMEK 1: sort [< vstup] =======================
    if ( fork() == 0 )
    {
        if ( chyby )  presmeruj( chyby, O_WRONLY | O_CREAT | O_APPEND, 2 );
        if ( vstup )  presmeruj( vstup, O_RDONLY, 0 );     // jinak zdedeny stdin

        dup2( roura1[ 1 ], 1 );
        zavri_roury();

        execlp( "sort", "sort", NULL );
        perror( "sort" );
        exit( 127 );
    }

    // ===================== POTOMEK 2: nl -s oddelovac ======================
    if ( fork() == 0 )
    {
        if ( chyby )  presmeruj( chyby, O_WRONLY | O_CREAT | O_APPEND, 2 );

        dup2( roura1[ 0 ], 0 );
        dup2( roura2[ 1 ], 1 );
        zavri_roury();

        execlp( "nl", "nl", "-s", oddelovac, NULL );
        perror( "nl" );
        exit( 127 );
    }

    // ===================== POTOMEK 3: tr a-z A-Z [> vystup] ================
    if ( fork() == 0 )
    {
        if ( chyby )  presmeruj( chyby, O_WRONLY | O_CREAT | O_APPEND, 2 );
        if ( vystup ) presmeruj( vystup, priznaky_vystupu, 1 );

        dup2( roura2[ 0 ], 0 );
        zavri_roury();

        execlp( "tr", "tr", "a-z", "A-Z", NULL );
        perror( "tr" );
        exit( 127 );
    }

    // ===================== RODIC ===========================================
    zavri_roury();

    for ( int i = 0; i < 3; i++ )
        wait( NULL );

    return 0;
}
