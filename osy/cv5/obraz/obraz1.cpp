#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#define POCET_ROUR 3

static int roury[ POCET_ROUR ][ 2 ];

static void zavri_roury( void )
{
    for ( int i = 0; i < POCET_ROUR; i++ )
    {
        close( roury[ i ][ 0 ] );
        close( roury[ i ][ 1 ] );
    }
}

int main( int argc, char **argv )
{
    if ( argc != 2 && argc != 3 )
    {
        fprintf( stderr, "Pouziti: %s vstup [vystup]\n", argv[ 0 ] );
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

        execlp( "rev", "rev", NULL );
        perror( "rev" );
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

        if ( argc == 3 )
        {
            int y = open( argv[ 2 ], O_WRONLY | O_CREAT | O_TRUNC, 0644 );

            if ( y < 0 )
            {
                perror( argv[ 2 ] );
                exit( 1 );
            }

            dup2( y, 1 );
            close( y );
        }

        zavri_roury();

        execlp( "basenc", "basenc", "-i", "-d", "--base16", NULL );
        perror( "basenc" );
        exit( 127 );
    }

    zavri_roury();

    for ( int i = 0; i < 4; i++ )
        wait( NULL );

    return 0;
}
