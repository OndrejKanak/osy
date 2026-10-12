// ============================================================================
// v6_stavy.cpp - varianta 6: rodic hlida, jak potomci skoncili
//
// Pouziti:  ./v6_stavy soubor
//
// Stejna kolona jako kolona1:   sort < soubor | nl -s ". " | tr a-z A-Z
//
// Navic rodic pres waitpid() zjisti u kazdeho potomka, jak skoncil, vypise
// to na stderr a sam skonci s kodem 1, kdyz nektery potomek selhal
// (podobne jako  set -o pipefail  v bashi).
//
//   exit 0         ... vse v poradku
//   exit 1         ... potomek 1 nemohl otevrit soubor
//   exit 127       ... exec selhal - program nebyl nalezen
//   signal 13      ... SIGPIPE - nekdo zavrel rouru, do ktere psal
//
// Vyzkousej:
//     ./v6_stavy names.txt > /dev/null           vsichni 0
//     ./v6_stavy neni.txt                        potomek 1 skonci s 1
//     PATH=/nikde ./v6_stavy names.txt           exec selze - 127
//     ./v6_stavy names.txt | head -3             tr dostane SIGPIPE
//
// Predmet: Operacni systemy
// ============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>       // fork, pipe, dup2, close, execlp
#include <fcntl.h>        // open
#include <sys/wait.h>     // waitpid, WIFEXITED, ...

static int roura1[ 2 ];
static int roura2[ 2 ];

static void zavri_roury( void )
{
    close( roura1[ 0 ] );
    close( roura1[ 1 ] );
    close( roura2[ 0 ] );
    close( roura2[ 1 ] );
}

int main( int argc, char **argv )
{
    if ( argc != 2 )
    {
        fprintf( stderr, "Pouziti: %s soubor\n", argv[ 0 ] );
        return 1;
    }

    if ( pipe( roura1 ) < 0 || pipe( roura2 ) < 0 )
    {
        perror( "pipe" );
        return 2;
    }

    pid_t potomci[ 3 ];

    // ===================== POTOMEK 1: sort < soubor ========================
    potomci[ 0 ] = fork();

    if ( potomci[ 0 ] == 0 )
    {
        int x = open( argv[ 1 ], O_RDONLY );

        if ( x < 0 )
        {
            perror( argv[ 1 ] );
            exit( 1 );
        }

        dup2( x, 0 );
        dup2( roura1[ 1 ], 1 );
        close( x );
        zavri_roury();

        execlp( "sort", "sort", NULL );
        perror( "sort" );
        exit( 127 );                // 127 = program nenalezen, jako v shellu
    }

    // ===================== POTOMEK 2: nl -s ". " ===========================
    potomci[ 1 ] = fork();

    if ( potomci[ 1 ] == 0 )
    {
        dup2( roura1[ 0 ], 0 );
        dup2( roura2[ 1 ], 1 );
        zavri_roury();

        execlp( "nl", "nl", "-s", ". ", NULL );
        perror( "nl" );
        exit( 127 );
    }

    // ===================== POTOMEK 3: tr a-z A-Z ===========================
    potomci[ 2 ] = fork();

    if ( potomci[ 2 ] == 0 )
    {
        dup2( roura2[ 0 ], 0 );
        zavri_roury();

        execlp( "tr", "tr", "a-z", "A-Z", NULL );
        perror( "tr" );
        exit( 127 );
    }

    // ===================== RODIC: zavre roury, ceka a hlasi ================
    zavri_roury();

    const char *jmena[ 3 ] = { "sort", "nl", "tr" };
    int         chyba      = 0;

    for ( int i = 0; i < 3; i++ )
    {
        int stav;

        if ( potomci[ i ] < 0 || waitpid( potomci[ i ], &stav, 0 ) < 0 )
        {
            fprintf( stderr, "rodic: potomek %d (%s) se nepodarilo vytvorit\n", i + 1, jmena[ i ] );
            chyba = 1;
            continue;
        }

        if ( WIFEXITED( stav ) )
        {
            fprintf( stderr, "rodic: potomek %d (%s) skoncil s kodem %d\n",
                     i + 1, jmena[ i ], WEXITSTATUS( stav ) );

            if ( WEXITSTATUS( stav ) != 0 )
                chyba = 1;
        }
        else if ( WIFSIGNALED( stav ) )
        {
            fprintf( stderr, "rodic: potomek %d (%s) ukoncen signalem %d (%s)\n",
                     i + 1, jmena[ i ], WTERMSIG( stav ), strsignal( WTERMSIG( stav ) ) );
            chyba = 1;
        }
    }

    return chyba;
}
