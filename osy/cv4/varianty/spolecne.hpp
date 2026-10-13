// ============================================================================
// spolecne.hpp - pomocne funkce sdilene vsemi variantami kalendare
//
//   nahodne_datum()  ... vylosuje datum a zapise ho jako "den.mesic"
//   doplnit_jmeno()  ... z "den.mesic" udela "den.mesic. Jmeno"
//   zapis_vse()      ... write(), ktery zapise cely retezec
//   nacti_cisla()    ... zpracovani parametru M N
//
// Seznam svatku je z pripravy: ../kalendar/svatky.hpp
//
// Predmet: Operacni systemy
// ============================================================================

#ifndef SPOLECNE_HPP
#define SPOLECNE_HPP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>

#include "svatky.hpp"

// ---------------------------------------------------------------------------
// Vylosuje nahodne datum a zapise ho do bufferu jako "den.mesic" (bez '\n').
// Kazdy proces si musi sam zavolat srand() - jinak by vsichni generovali totez.
// ---------------------------------------------------------------------------
static inline void nahodne_datum( char *out, size_t size )
{
    int mesic = rand() % 12;                                // 0..11
    int den   = rand() % g_dni_v_mesici[ mesic ] + 1;       // 1..pocet dni

    snprintf( out, size, "%d.%d", den, mesic + 1 );
}

// ---------------------------------------------------------------------------
// Z "den.mesic" (pripadne s '\n' na konci) udela "den.mesic. Jmeno".
// ---------------------------------------------------------------------------
static inline void doplnit_jmeno( const char *den_mesic, char *out, size_t size )
{
    char datum[ 32 ];

    // "%.*s" vezme jen cast retezce pred pripadnym '\n'
    snprintf( datum, sizeof( datum ), "%.*s.",
              (int) strcspn( den_mesic, "\n" ), den_mesic );

    snprintf( out, size, "%s %s", datum, najdi_svatek( datum ) );
}

// ---------------------------------------------------------------------------
// Zapise cely retezec do deskriptoru (write() muze zapsat mene, nez se chce).
// Vraci 0 pri uspechu, -1 pri chybe.
// ---------------------------------------------------------------------------
static inline int zapis_vse( int fd, const char *text )
{
    size_t delka = strlen( text );

    while ( delka > 0 )
    {
        ssize_t n = write( fd, text, delka );

        if ( n < 0 )
        {
            if ( errno == EINTR )
                continue;

            return -1;
        }

        text  += n;
        delka -= (size_t) n;
    }

    return 0;
}

// ---------------------------------------------------------------------------
// Prevede parametr na cele cislo >= minimum. Pri chybe vypise hlasku a skonci.
// ---------------------------------------------------------------------------
static inline int nacti_cislo( const char *text, int minimum, const char *popis )
{
    char *konec;
    long  hodnota = strtol( text, &konec, 10 );

    if ( *text == 0 || *konec != 0 || hodnota < minimum || hodnota > 1000000 )
    {
        fprintf( stderr, "Chyba: %s musi byt cele cislo >= %d (zadano '%s').\n",
                 popis, minimum, text );
        exit( 1 );
    }

    return (int) hodnota;
}

#endif // SPOLECNE_HPP
