#include <stdio.h>
#include <stdlib.h>

int main( void )
{
    char   *radek    = NULL;
    size_t  kapacita = 0;
    ssize_t delka;

    while ( ( delka = getline( &radek, &kapacita, stdin ) ) != -1 )
    {
        int konec_radku = ( radek[ delka - 1 ] == '\n' );

        if ( konec_radku )
            delka--;

        for ( ssize_t i = delka - 1; i >= 0; i-- )
            putchar( radek[ i ] );

        if ( konec_radku )
            putchar( '\n' );
    }

    free( radek );

    return 0;
}
