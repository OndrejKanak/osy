// ============================================================================
// dyn_lib1/compute.cpp - DYNAMICKA knihovna libcompute.so, varianta DOPOCET
//
// Funkce compute() cte vstup po radcich, secte cisla na radku a puvodni
// radek i s doplnenym souctem na konci vypise na standardni vystup.
//
// Preklad:   g++ -fPIC -c compute.cpp -o compute.o
// Knihovna:  g++ -shared compute.o -o libcompute.so
//
// -fPIC = Position Independent Code. Sdilena knihovna se v kazdem procesu
//         muze namapovat na jinou adresu, takze kod nesmi obsahovat pevne
//         absolutni adresy - adresuje se relativne vuci sobe.
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

    while ( read_line( in, &line, &cap ) >= 0 )
    {
        long  sum   = 0;
        int   count = 0;
        char *pos   = line;

        // Postupne vytahame z radku vsechna cisla.
        for ( ;; )
        {
            char *end   = NULL;
            long  value = strtol( pos, &end, 10 );

            if ( end == pos )    // uz tam zadne dalsi cislo neni
                break;

            sum += value;
            count++;
            pos = end;
        }

        if ( count > 0 )
            printf( "%s %ld\n", line, sum );   // puvodni radek + dopocitany soucet
        else
            printf( "%s\n", line );            // radek bez cisel opiseme beze zmeny
    }

    free( line );

    return ferror( in ) ? -1 : 0;
}
