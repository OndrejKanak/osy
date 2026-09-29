// ============================================================================
// dyn_lib2/compute.cpp - DYNAMICKA knihovna libcompute.so, varianta KONTROLA
//
// Funkce compute() cte vstup po radcich a overuje, ze POSLEDNI cislo na radku
// je souctem vsech cisel pred nim. U vadneho radku vypise jeho poradove
// cislo, nalezenou hodnotu a ocekavany soucet.
//
// Jmeno souboru knihovny i jmeno funkce je zamerne stejne jako v dyn_lib1 -
// z pohledu programu main2 jsou obe knihovny zcela zamenitelne.
//
// Preklad:   g++ -fPIC -c compute.cpp -o compute.o
// Knihovna:  g++ -shared compute.o -o libcompute.so
// ============================================================================

#include "compute.h"

#include <stdlib.h>     // realloc, free, strtol
#include <string.h>     // strlen

// ---------------------------------------------------------------------------
// Precte jeden radek LIBOVOLNE delky do dynamicky alokovaneho bufferu.
//
// Buffer si funkce spravuje sama - pred prvnim volanim staci nastavit
// *buf na NULL a *cap na 0, po skonceni cteni uvolnit buffer pomoci free().
//
// Vraci delku radku uz BEZ koncovych znaku '\n' a '\r', nebo -1 na konci
// vstupu. Dela tedy totez, co POSIX funkce getline(), jen prenositelne.
// ---------------------------------------------------------------------------
static long read_line( FILE *in, char **buf, size_t *cap )
{
    size_t len = 0;

    for ( ;; )
    {
        if ( len + 1 >= *cap )      // v bufferu uz neni misto, zvetsime ho
        {
            size_t newcap = *cap ? *cap * 2 : 256;
            char  *tmp    = (char *) realloc( *buf, newcap );

            if ( tmp == NULL )
                return -1;

            *buf = tmp;
            *cap = newcap;
        }

        if ( fgets( *buf + len, (int) ( *cap - len ), in ) == NULL )
            break;                  // konec souboru nebo chyba cteni

        len += strlen( *buf + len );

        if ( len > 0 && ( *buf )[ len - 1 ] == '\n' )
            break;                  // mame cely radek vcetne jeho konce
    }

    if ( len == 0 )
        return -1;                  // uz se nic neprecetlo

    // Odstranime konec radku, vcetne pripadneho CR ze souboru z Windows.
    while ( len > 0 && ( ( *buf )[ len - 1 ] == '\n' || ( *buf )[ len - 1 ] == '\r' ) )
        ( *buf )[ --len ] = 0;

    return (long) len;
}

int compute( FILE *in )
{
    char   *line = NULL;
    size_t  cap  = 0;

    int lineno  = 0;    // poradove cislo radku ve vstupu, cislujeme od 1
    int checked = 0;    // pocet skutecne zkontrolovanych radku
    int bad     = 0;    // pocet vadnych radku

    while ( read_line( in, &line, &cap ) >= 0 )
    {
        lineno++;

        long  total = 0;    // soucet VSECH cisel na radku
        long  last  = 0;    // posledni cislo na radku = deklarovany soucet
        int   count = 0;
        char *pos   = line;

        for ( ;; )
        {
            char *end   = NULL;
            long  value = strtol( pos, &end, 10 );

            if ( end == pos )
                break;

            total += value;
            last   = value;
            count++;
            pos = end;
        }

        if ( count == 0 )       // prazdny radek jen preskocime
            continue;           // (cislovani radku tim zustava spravne)

        checked++;

        if ( count < 2 )
        {
            printf( "Radek %d: je na nem jen jedno cislo, soucet chybi.\n", lineno );
            bad++;
            continue;
        }

        // Ocekavany soucet = soucet vsech cisel na radku bez toho posledniho.
        long expected = total - last;

        if ( last != expected )
        {
            printf( "Radek %d: spatny soucet - nalezeno %ld, ocekavano %ld.\n",
                    lineno, last, expected );
            bad++;
        }
    }

    free( line );

    if ( ferror( in ) )
        return -1;

    if ( bad == 0 )
        printf( "Kontrola OK: zkontrolovano %d radku, vsechny soucty sedi.\n", checked );
    else
        printf( "Kontrola NEPROSLA: vadnych radku %d z %d.\n", bad, checked );

    return bad;
}
