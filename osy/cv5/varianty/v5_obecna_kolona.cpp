// ============================================================================
// v5_obecna_kolona.cpp - varianta 5: libovolna kolona prikazu z parametru
//
// Pouziti:  ./v5_obecna_kolona vstup prikaz [args] , prikaz [args] , ...
//
// Prikazy se oddeluji samostatnou carkou ",". Priklad:
//
//     ./v5_obecna_kolona names.txt sort , nl -s ". " , tr a-z A-Z
//
// dela totez co  sort < names.txt | nl -s ". " | tr a-z A-Z
//
// Proc carka a ne "|": znak | by si vzal shell a udelal by rouru sam.
// Proc ne jeden retezec "nl -s '. '": museli bychom ho rozdelit na slova
// a resit uvozovky. Takhle argumenty rozdeli shell a program je dostane
// v argv uz hotove - staci misto carky dat NULL a predat cast argv do
// execvp().
//
// Pocet potomku i rour je promenny, proto se vytvareji v CYKLU:
//
//   vstup -> [ prikaz 0 ] -r-> [ prikaz 1 ] -r-> ... -> [ prikaz N-1 ] -> stdout
//
// Rodic si drzi jen cteci konec predchozi roury ('predchozi'). Kazdy
// potomek ma stdin = predchozi roura (u prvniho soubor) a stdout = nova
// roura (u posledniho obrazovka).
//
// Predmet: Operacni systemy
// ============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>       // fork, pipe, dup2, close, execvp
#include <fcntl.h>        // open
#include <sys/wait.h>     // wait

int main( int argc, char **argv )
{
    if ( argc < 3 )
    {
        fprintf( stderr, "Pouziti: %s vstup prikaz [args] , prikaz [args] , ...\n", argv[ 0 ] );
        fprintf( stderr, "Priklad: %s names.txt sort , nl -s \". \" , tr a-z A-Z\n", argv[ 0 ] );
        return 1;
    }

    // Kde zacina kazdy prikaz: carky nahradime NULL, takze kazdy usek argv
    // je hotove pole argumentu ukoncene NULL - presne to chce execvp().
    // Posledni prikaz konci argv[ argc ], ktery je NULL vzdy.
    char **prikazy[ 64 ];
    int    pocet = 0;

    prikazy[ pocet++ ] = &argv[ 2 ];

    for ( int i = 2; i < argc; i++ )
    {
        if ( strcmp( argv[ i ], "," ) == 0 )
        {
            if ( i + 1 >= argc || pocet >= 64 )
            {
                fprintf( stderr, "Chyba: za carkou chybi prikaz (nebo je jich prilis).\n" );
                return 1;
            }

            argv[ i ]          = NULL;
            prikazy[ pocet++ ] = &argv[ i + 1 ];
        }
    }

    // Prvni potomek cte ze souboru - otevreme ho tady, potomek ho zdedi.
    int predchozi = open( argv[ 1 ], O_RDONLY );

    if ( predchozi < 0 )
    {
        perror( argv[ 1 ] );
        return 2;
    }

    for ( int i = 0; i < pocet; i++ )
    {
        int posledni = ( i == pocet - 1 );
        int roura[ 2 ];

        if ( !posledni && pipe( roura ) < 0 )
        {
            perror( "pipe" );
            return 2;
        }

        // ================= POTOMEK i ======================================
        if ( fork() == 0 )
        {
            dup2( predchozi, 0 );           // stdin = soubor / predchozi roura
            close( predchozi );

            if ( !posledni )
            {
                dup2( roura[ 1 ], 1 );      // stdout = nova roura
                close( roura[ 0 ] );
                close( roura[ 1 ] );
            }

            execvp( prikazy[ i ][ 0 ], prikazy[ i ] );
            perror( prikazy[ i ][ 0 ] );
            exit( 127 );
        }

        // ================= RODIC ==========================================
        // Predchozi roura uz je predana potomkovi - rodic ji zavre.
        // Zapisovaci konec nove roury patri potomkovi - taky zavrit.
        // Cteci konec si nechame pro dalsiho potomka.
        close( predchozi );

        if ( !posledni )
        {
            close( roura[ 1 ] );
            predchozi = roura[ 0 ];
        }
    }

    for ( int i = 0; i < pocet; i++ )
        wait( NULL );

    return 0;
}
