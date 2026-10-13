// ============================================================================
// vlastni.hpp - vlastni implementace beznych filtru
//
// Kazda funkce cte stdin a pise stdout - presne jako program, ktery
// nahrazuje. V potomkovi se tedy pouzije misto execlp() po stejnych dup2():
//
//     dup2( roura_in[ 0 ], 0 );
//     dup2( roura_out[ 1 ], 1 );
//     ...zavrit roury...
//     moje_tr();                  // misto execlp( "tr", "tr", "a-z", "A-Z", NULL )
//     exit( 0 );                  // exit() vyprazdni stdout buffer
//
//   moje_nl()          nl -s ". "
//   moje_tr()          tr a-z A-Z
//   moje_head( n )     head -n N
//   moje_grep( text )  grep -F text      (hleda presny text, ne regularni vyraz)
//   moje_wc_l()        wc -l
//   moje_uniq()        uniq
//   moje_sort()        sort
//
// Radky se ctou bud znak po znaku (getchar), nebo funkci getline(), ktera
// si buffer sama zvetsuje - delka radku tak neni nicim omezena.
//
// Predmet: Operacni systemy
// ============================================================================

#ifndef VLASTNI_HPP
#define VLASTNI_HPP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

// ---------------------------------------------------------------------------
// nl -s ". "  - stejne jako v kolona2.cpp
// ---------------------------------------------------------------------------
static inline void moje_nl( void )
{
    int cislo         = 0;
    int zacatek_radku = 1;
    int c;

    while ( ( c = getchar() ) != EOF )
    {
        if ( zacatek_radku )
        {
            if ( c == '\n' )
                printf( "%6s  ", "" );              // prazdny radek: 8 mezer
            else
                printf( "%6d. ", ++cislo );
        }

        putchar( c );
        zacatek_radku = ( c == '\n' );
    }

    if ( !zacatek_radku )
        putchar( '\n' );
}

// ---------------------------------------------------------------------------
// tr a-z A-Z  - jen pismena a-z, ostatni bajty (i diakritika) beze zmeny
// ---------------------------------------------------------------------------
static inline void moje_tr( void )
{
    int c;

    while ( ( c = getchar() ) != EOF )
    {
        if ( c >= 'a' && c <= 'z' )
            c = c - 'a' + 'A';

        putchar( c );
    }
}

// ---------------------------------------------------------------------------
// head -n N  - vypise prvnich N radku a skonci
// ---------------------------------------------------------------------------
static inline void moje_head( int n )
{
    int radku = 0;
    int c;

    while ( radku < n && ( c = getchar() ) != EOF )
    {
        putchar( c );

        if ( c == '\n' )
            radku++;
    }
}

// ---------------------------------------------------------------------------
// wc -l  - pocet znaku '\n'
// ---------------------------------------------------------------------------
static inline void moje_wc_l( void )
{
    long radku = 0;
    int  c;

    while ( ( c = getchar() ) != EOF )
        if ( c == '\n' )
            radku++;

    printf( "%ld\n", radku );
}

// ---------------------------------------------------------------------------
// grep -F text  - vypise radky, ktere obsahuji zadany text
// ---------------------------------------------------------------------------
static inline void moje_grep( const char *text )
{
    char   *radek = NULL;
    size_t  kapacita = 0;
    ssize_t delka;

    while ( ( delka = getline( &radek, &kapacita, stdin ) ) != -1 )
    {
        if ( strstr( radek, text ) != NULL )
        {
            fputs( radek, stdout );

            if ( radek[ delka - 1 ] != '\n' )   // posledni radek bez '\n'
                putchar( '\n' );
        }
    }

    free( radek );
}

// ---------------------------------------------------------------------------
// uniq  - vynecha radek, ktery je stejny jako predchozi
// ---------------------------------------------------------------------------
static inline void moje_uniq( void )
{
    char   *radek = NULL, *predchozi = NULL;
    size_t  kapacita = 0;
    ssize_t delka;

    while ( ( delka = getline( &radek, &kapacita, stdin ) ) != -1 )
    {
        if ( radek[ delka - 1 ] == '\n' )       // porovnavat bez '\n'
            radek[ delka - 1 ] = 0;

        if ( predchozi == NULL || strcmp( radek, predchozi ) != 0 )
        {
            printf( "%s\n", radek );
            free( predchozi );
            predchozi = strdup( radek );
        }
    }

    free( radek );
    free( predchozi );
}

// ---------------------------------------------------------------------------
// sort  - nacte vsechny radky do pole a seradi je qsort()
//
// Program sort radi podle jazykoveho nastaveni (locale): pouziva strcoll()
// a teprve kdyz jsou radky "stejne", rozhodne obycejne strcmp(). Aby
// vysledek sedel, je potreba nejdriv setlocale( LC_ALL, "" ).
// ---------------------------------------------------------------------------
static inline int porovnej_radky( const void *a, const void *b )
{
    const char *x = *(const char * const *) a;
    const char *y = *(const char * const *) b;

    int r = strcoll( x, y );

    return r != 0 ? r : strcmp( x, y );
}

static inline void moje_sort( void )
{
    setlocale( LC_ALL, "" );

    char  **radky    = NULL;
    size_t  pocet    = 0;
    size_t  kapacita = 0;

    char   *radek = NULL;
    size_t  velikost = 0;
    ssize_t delka;

    while ( ( delka = getline( &radek, &velikost, stdin ) ) != -1 )
    {
        if ( radek[ delka - 1 ] == '\n' )
            radek[ delka - 1 ] = 0;

        if ( pocet == kapacita )                // pole je plne - zdvojnasobit
        {
            kapacita = kapacita ? 2 * kapacita : 1024;
            radky    = (char **) realloc( radky, kapacita * sizeof( char * ) );
        }

        radky[ pocet++ ] = strdup( radek );
    }

    qsort( radky, pocet, sizeof( char * ), porovnej_radky );

    for ( size_t i = 0; i < pocet; i++ )
    {
        printf( "%s\n", radky[ i ] );
        free( radky[ i ] );
    }

    free( radky );
    free( radek );
}

#endif // VLASTNI_HPP
