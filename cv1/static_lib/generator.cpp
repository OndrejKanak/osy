// ============================================================================
// generator.cpp - implementace STATICKE knihovny libgenerator.a
//
// Preklad:   g++ -c generator.cpp -o generator.o
// Knihovna:  ar rcs libgenerator.a generator.o
//
// Staticka knihovna je jen archiv (kontejner) prelozenych .o souboru.
// Pri linkovani programu se z nej VYKOPIRUJI pouzite funkce primo do
// vysledneho spustitelneho souboru - vyslednou binarku uz pak knihovna
// nezajima a za behu ji nepotrebuje.
// ============================================================================

#include "generator.h"

#include <stdlib.h>     // rand, srand
#include <time.h>       // time
#include <unistd.h>     // getpid

void gen_init( unsigned int seed )
{
    if ( seed == 0 )
        seed = (unsigned int) time( NULL ) ^ ( (unsigned int) getpid() << 16 );

    srand( seed );
}

int gen_number( void )
{
    // rand() vraci 0 .. RAND_MAX; modulem zuzime na sirku rozsahu
    // a posunem na GEN_MIN dostaneme <GEN_MIN, GEN_MAX>.
    return GEN_MIN + rand() % ( GEN_MAX - GEN_MIN + 1 );
}

void gen_line( FILE *out, int count )
{
    for ( int i = 0; i < count; i++ )
        fprintf( out, "%s%d", i ? " " : "", gen_number() );

    fprintf( out, "\n" );
}

void gen_matrix( FILE *out, int rows, int cols )
{
    for ( int r = 0; r < rows; r++ )
        gen_line( out, cols );
}
