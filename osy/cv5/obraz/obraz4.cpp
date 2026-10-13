#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#define POCET_ROUR 4

static int roury[ POCET_ROUR ][ 2 ];

static void zavri_roury( void )
{
    for ( int i = 0; i < POCET_ROUR; i++ )
    {
        close( roury[ i ][ 0 ] );
        close( roury[ i ][ 1 ] );
    }
}

static int hodnota_hex( int c )
{
    if ( c >= '0' && c <= '9' ) return c - '0';
    if ( c >= 'A' && c <= 'F' ) return c - 'A' + 10;
    if ( c >= 'a' && c <= 'f' ) return c - 'a' + 10;
    return -1;
}

static void hex_na_bajty( void )
{
    int horni = -1;
    int c;

    while ( ( c = getchar() ) != EOF )
    {
        int h = hodnota_hex( c );

        if ( h < 0 )
            continue;

        if ( horni < 0 )
        {
            horni = h;
        }
        else
        {
            putchar( horni * 16 + h );
            horni = -1;
        }
    }
}

int main( int argc, char **argv )
{
    if ( argc != 2 )
    {
        fprintf( stderr, "Pouziti: %s vstup\n", argv[ 0 ] );
        return 1;
    }

    for ( int i = 0; i < POCET_ROUR; i++ )
    {
        if ( pipe( roury[ i ] ) < 0 )
        {
            perror( "pipe" );
            return 2;
        }
    }

    if ( fork() == 0 )
    {
        int x = open( argv[ 1 ], O_RDONLY );

        if ( x < 0 )
        {
            perror( argv[ 1 ] );
            exit( 1 );
        }

        dup2( x, 0 );
        dup2( roury[ 0 ][ 1 ], 1 );
        close( x );
        zavri_roury();

        execlp( "./myrev", "myrev", NULL );
        perror( "./myrev" );
        exit( 127 );
    }

    if ( fork() == 0 )
    {
        dup2( roury[ 0 ][ 0 ], 0 );
        dup2( roury[ 1 ][ 1 ], 1 );
        zavri_roury();

        execlp( "sort", "sort", "-n", NULL );
        perror( "sort" );
        exit( 127 );
    }

    if ( fork() == 0 )
    {
        dup2( roury[ 1 ][ 0 ], 0 );
        dup2( roury[ 2 ][ 1 ], 1 );
        zavri_roury();

        execlp( "cut", "cut", "-d", ".", "-f", "2", NULL );
        perror( "cut" );
        exit( 127 );
    }

    if ( fork() == 0 )
    {
        dup2( roury[ 2 ][ 0 ], 0 );
        dup2( roury[ 3 ][ 1 ], 1 );
        zavri_roury();

        hex_na_bajty();
        exit( 0 );
    }

    if ( fork() == 0 )
    {
        dup2( roury[ 3 ][ 0 ], 0 );
        zavri_roury();

        execlp( "display", "display", "-", NULL );
        perror( "display" );
        exit( 127 );
    }

    zavri_roury();

    for ( int i = 0; i < 5; i++ )
        wait( NULL );

    return 0;
}
