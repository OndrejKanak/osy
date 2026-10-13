// ============================================================================
// v1_exec.cpp - varianta 1: treti potomek je externi program (dup2 + exec)
//
// Pouziti:  ./v1_exec M N [prikaz argumenty...]
//
//     M ... kolik datumu se vygeneruje
//     N ... kolik datumu za sekundu
//     prikaz ... program, ktery spusti potomek 3; vychozi je
//                sort -t. -k2,2n -k1,1n   (seradi podle mesice a dne)
//
//   potomek 1 ... generuje "den.mesic\n"                    -> roura A
//   potomek 2 ... roura A -> "den.mesic. Jmeno\n"           -> roura B
//   potomek 3 ... roura B presmeruje na svuj stdin (dup2) a prepise se
//                 programem sort pomoci execvp()             -> stdout
//   rodic     ... jen vytvori roury a potomky, zavre roury a ceka
//
// Priklady:
//     ./v1_exec 20 20                      serazeny vypis
//     ./v1_exec 20 20 cat -n               ocislovane radky
//     ./v1_exec 500 1000 wc -l             spocita radky (500)
//     ./v1_exec 50 100 grep Jan            jen radky obsahujici "Jan"
//
// Predmet: Operacni systemy
// ============================================================================

#include <time.h>
#include <sys/wait.h>

#include "spolecne.hpp"

int main( int argc, char **argv )
{
    if ( argc < 3 )
    {
        fprintf( stderr, "Pouziti: %s M N [prikaz argumenty...]\n", argv[ 0 ] );
        fprintf( stderr, "Bez prikazu se spusti: sort -t. -k2,2n -k1,1n\n" );
        return 1;
    }

    int pocet      = nacti_cislo( argv[ 1 ], 0, "M" );
    int za_sekundu = nacti_cislo( argv[ 2 ], 1, "N" );

    // Prikaz pro potomka 3: bud zbytek prikazove radky, nebo vychozi sort.
    // execvp() chce pole argumentu ukoncene NULL - argv takove je.
    const char *vychozi[] = { "sort", "-t.", "-k2,2n", "-k1,1n", NULL };
    char      **prikaz    = ( argc > 3 ) ? &argv[ 3 ] : (char **) vychozi;

    int roura_a[ 2 ];
    int roura_b[ 2 ];

    if ( pipe( roura_a ) < 0 || pipe( roura_b ) < 0 )
    {
        perror( "pipe" );
        return 2;
    }

    // ===================== POTOMEK 1: generator ============================
    pid_t p1 = fork();

    if ( p1 == 0 )
    {
        close( roura_a[ 0 ] );
        close( roura_b[ 0 ] );
        close( roura_b[ 1 ] );

        srand( (unsigned) time( NULL ) ^ (unsigned) getpid() );

        for ( int i = 0; i < pocet; i++ )
        {
            char datum[ 16 ], radek[ 20 ];

            nahodne_datum( datum, sizeof( datum ) );
            snprintf( radek, sizeof( radek ), "%s\n", datum );

            if ( zapis_vse( roura_a[ 1 ], radek ) != 0 )
                break;

            usleep( 1000000 / za_sekundu );
        }

        close( roura_a[ 1 ] );
        exit( 0 );
    }

    // ===================== POTOMEK 2: doplni jmena =========================
    pid_t p2 = fork();

    if ( p2 == 0 )
    {
        close( roura_a[ 1 ] );
        close( roura_b[ 0 ] );

        FILE *in = fdopen( roura_a[ 0 ], "r" );
        char  radek[ 64 ], vystup[ 128 ];

        while ( in != NULL && fgets( radek, sizeof( radek ), in ) != NULL )
        {
            doplnit_jmeno( radek, vystup, sizeof( vystup ) - 1 );
            strcat( vystup, "\n" );

            if ( zapis_vse( roura_b[ 1 ], vystup ) != 0 )
                break;
        }

        close( roura_b[ 1 ] );
        exit( 0 );
    }

    // ===================== POTOMEK 3: externi program ======================
    pid_t p3 = fork();

    if ( p3 == 0 )
    {
        close( roura_a[ 0 ] );
        close( roura_a[ 1 ] );
        close( roura_b[ 1 ] );

        // dup2( stary, novy ): deskriptor 0 (stdin) bude ukazovat tam, kam
        // roura_b[ 0 ]. Puvodni roura_b[ 0 ] uz pak neni potreba - zavreme ji,
        // aby rouru nedrzely otevrenou dva deskriptory.
        dup2( roura_b[ 0 ], STDIN_FILENO );
        close( roura_b[ 0 ] );

        // execvp() nahradi cely program potomka programem 'prikaz'. Hleda ho
        // v PATH (proto 'p'), argumenty bere z pole (proto 'v'). Otevrene
        // deskriptory - vcetne noveho stdin - zustavaji zachovane.
        execvp( prikaz[ 0 ], prikaz );

        // Sem se program dostane JEN kdyz exec selze.
        perror( prikaz[ 0 ] );
        exit( 127 );
    }

    // ===================== RODIC: uklidi a ceka ============================
    close( roura_a[ 0 ] );
    close( roura_a[ 1 ] );
    close( roura_b[ 0 ] );
    close( roura_b[ 1 ] );

    pid_t potomci[ 3 ] = { p1, p2, p3 };

    for ( int i = 0; i < 3; i++ )
    {
        int stav;

        if ( potomci[ i ] > 0 && waitpid( potomci[ i ], &stav, 0 ) > 0 )
            fprintf( stderr, "rodic: potomek %d (%d) skoncil se stavem %d\n",
                     i + 1, (int) potomci[ i ],
                     WIFEXITED( stav ) ? WEXITSTATUS( stav ) : -1 );
    }

    return 0;
}
