// ============================================================================
// myls.cpp - vlastni zjednodusena verze prikazu ls
//
// Pouziti:  ./myls [-s] [-t] [-r] soubor ...
//
//     -s   velikost souboru v bajtech
//     -t   cas posledni zmeny obsahu (st_mtime, totez co ukazuje ls -l)
//     -r   pristupova prava ve tvaru  -rw-r--r--
//
// Informace se vypisuji v TOM PORADI, v jakem byly prepinace zadany:
//     ./myls -s -r soubor      ->   velikost, prava, jmeno
//     ./myls -r -s soubor      ->   prava, velikost, jmeno
//
// Jmeno souboru je vzdy posledni sloupec vpravo. Soubory, ktere se nepodarilo
// najit, se vypisi v souhrnu na konci.
//
// Predmet: Operacni systemy
// ============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>       // getopt, optind
#include <sys/stat.h>     // lstat, struct stat, makra S_IS* a S_IR*
#include <time.h>         // localtime, strftime

#define MAX_OPTIONS  32   // rozumny strop na pocet zadanych prepinacu

// ---------------------------------------------------------------------------
// Prevede prava ze st_mode na retezec ve tvaru "-rwxr-xr-x" (10 znaku + '\0').
//
// Prvni znak je typ souboru, pak vzdy tri trojice rwx pro vlastnika,
// skupinu a ostatni. Na mistech x se navic zobrazuji zvlastni bity:
//     s / S  ... setuid (u vlastnika) nebo setgid (u skupiny)
//     t / T  ... sticky bit (u ostatnich)
// Velke pismeno znamena, ze bit je nastaven, ale prislusne x nikoli.
// ---------------------------------------------------------------------------
static void format_mode( mode_t mode, char *out )
{
    char type = '?';

    if      ( S_ISREG(  mode ) ) type = '-';
    else if ( S_ISDIR(  mode ) ) type = 'd';
    else if ( S_ISLNK(  mode ) ) type = 'l';
    else if ( S_ISCHR(  mode ) ) type = 'c';
    else if ( S_ISBLK(  mode ) ) type = 'b';
    else if ( S_ISFIFO( mode ) ) type = 'p';
    else if ( S_ISSOCK( mode ) ) type = 's';

    out[ 0 ] = type;

    out[ 1 ] = ( mode & S_IRUSR ) ? 'r' : '-';
    out[ 2 ] = ( mode & S_IWUSR ) ? 'w' : '-';
    out[ 3 ] = ( mode & S_ISUID ) ? ( ( mode & S_IXUSR ) ? 's' : 'S' )
                                  : ( ( mode & S_IXUSR ) ? 'x' : '-' );

    out[ 4 ] = ( mode & S_IRGRP ) ? 'r' : '-';
    out[ 5 ] = ( mode & S_IWGRP ) ? 'w' : '-';
    out[ 6 ] = ( mode & S_ISGID ) ? ( ( mode & S_IXGRP ) ? 's' : 'S' )
                                  : ( ( mode & S_IXGRP ) ? 'x' : '-' );

    out[ 7 ] = ( mode & S_IROTH ) ? 'r' : '-';
    out[ 8 ] = ( mode & S_IWOTH ) ? 'w' : '-';
    out[ 9 ] = ( mode & S_ISVTX ) ? ( ( mode & S_IXOTH ) ? 't' : 'T' )
                                  : ( ( mode & S_IXOTH ) ? 'x' : '-' );

    out[ 10 ] = 0;
}

// ---------------------------------------------------------------------------
// Prevede cas z struct stat na retezec "RRRR-MM-DD HH:MM:SS" (19 znaku).
//
// localtime() prepocte cas v sekundach od 1.1.1970 na mistni cas rozlozeny
// do polozek (rok, mesic, den, ...), strftime() ho pak naformatuje.
// Obe funkce patri do skupiny popsane v  man asctime.
// ---------------------------------------------------------------------------
static void format_time( time_t when, char *out, size_t size )
{
    struct tm *tm = localtime( &when );

    if ( tm == NULL || strftime( out, size, "%Y-%m-%d %H:%M:%S", tm ) == 0 )
        snprintf( out, size, "%s", "?" );
}

static void usage( const char *program )
{
    fprintf( stderr, "Pouziti: %s [-s] [-t] [-r] soubor ...\n", program );
    fprintf( stderr, "   -s   velikost souboru v bajtech\n" );
    fprintf( stderr, "   -t   cas posledni zmeny obsahu\n" );
    fprintf( stderr, "   -r   pristupova prava\n" );
    fprintf( stderr, "Informace se vypisuji v poradi, v jakem jsou prepinace zadany.\n" );
    fprintf( stderr, "Priklad: %s -r -s -t *.cpp\n", program );
}

int main( int argc, char **argv )
{
    char options[ MAX_OPTIONS ];    // prepinace v poradi, jak byly zadany
    int  option_count = 0;

    // getopt() vraci prepinace presne v tom poradi, v jakem jsou na prikazove
    // radce - staci si je tedy postupne ukladat do pole.
    int c;

    opterr = 0;     // chybove hlasky si vypiseme sami

    while ( ( c = getopt( argc, argv, "str" ) ) != -1 )
    {
        if ( c == '?' )
        {
            fprintf( stderr, "Neznamy prepinac -%c\n\n", optopt );
            usage( argv[ 0 ] );
            return 1;
        }

        if ( option_count >= MAX_OPTIONS )
        {
            fprintf( stderr, "Chyba: prilis mnoho prepinacu (max %d).\n", MAX_OPTIONS );
            return 1;
        }

        options[ option_count++ ] = (char) c;
    }

    if ( optind >= argc )
    {
        fprintf( stderr, "Chyba: nezadan zadny soubor.\n\n" );
        usage( argv[ 0 ] );
        return 1;
    }

    // Seznam nenalezenych souboru. Staci pole ukazatelu do argv, retezce
    // samotne nikam kopirovat nemusime - argv zije po celou dobu behu.
    const char **missing = (const char **) malloc( (size_t) argc * sizeof( char * ) );

    if ( missing == NULL )
    {
        fprintf( stderr, "Chyba: nedostatek pameti.\n" );
        return 2;
    }

    int missing_count = 0;

    for ( int i = optind; i < argc; i++ )
    {
        const char *name = argv[ i ];
        struct stat st;

        // lstat() na rozdil od stat() nenasleduje symbolicke odkazy - ukaze
        // informace o odkazu samotnem, presne jako to dela ls -l.
        if ( lstat( name, &st ) != 0 )
        {
            missing[ missing_count++ ] = name;
            continue;
        }

        // Vypis polozek v poradi zadanych prepinacu.
        for ( int o = 0; o < option_count; o++ )
        {
            switch ( options[ o ] )
            {
                case 's':
                    printf( "%12lld ", (long long) st.st_size );
                    break;

                case 't':
                {
                    char buffer[ 32 ];
                    format_time( st.st_mtime, buffer, sizeof( buffer ) );
                    printf( "%-19s ", buffer );
                    break;
                }

                case 'r':
                {
                    char buffer[ 11 ];
                    format_mode( st.st_mode, buffer );
                    printf( "%-10s ", buffer );
                    break;
                }
            }
        }

        // Jmeno souboru je vzdy posledni sloupec vpravo.
        printf( "%s\n", name );
    }

    if ( missing_count > 0 )
    {
        printf( "\nNenalezene soubory (%d):\n", missing_count );

        for ( int i = 0; i < missing_count; i++ )
            printf( "    %s\n", missing[ i ] );
    }

    free( missing );

    return missing_count > 0 ? 1 : 0;
}
