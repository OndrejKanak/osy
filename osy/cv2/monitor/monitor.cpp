// ============================================================================
// monitor.cpp - vlastni obdoba prikazu  tail -f
//
// Pouziti:  ./monitor [-a] soubor
//
//     bez prepinace ... jako tail -f: preskoci stavajici obsah a vypisuje
//                       jen to, co do souboru nove pribude
//     -a             ... vypise nejdriv cely stavajici obsah a teprve pak
//                       sleduje prirustky
//
// Priklad:  ./gennum 10 60 > out.txt      (v prvnim terminalu)
//           ./monitor out.txt             (ve druhem terminalu)
//           truncate -s 0 out.txt         (ve tretim terminalu)
//
// Jak to funguje:
//   - soubor se otevre jednou pomoci open() a uz se NEZAVIRA
//   - jednou za sekundu se zavola fstat() nad otevrenym deskriptorem
//   - podle zjistene velikosti se bud doctou nove bajty pomoci read(),
//     nebo se pri zkraceni souboru skoci lseek() zpet na zacatek
//
// Obsah souboru jde na stdout, hlaseni o zmenach velikosti na stderr.
// Diky tomu se da obsah presmerovat  (./monitor out.txt > kopie.txt)
// a hlaseni pritom zustanou videt na terminalu.
//
// Predmet: Operacni systemy
// ============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#include <unistd.h>
#include <sys/stat.h>

#define BUFFER_SIZE    4096
#define POLL_SECONDS      1

static volatile sig_atomic_t running = 1;

static void on_signal( int sig )
{
    (void) sig;
    running = 0;
}

static void now_string( char *out, size_t size )
{
    time_t     now = time( NULL );
    struct tm *tm  = localtime( &now );

    if ( tm == NULL || strftime( out, size, "%Y-%m-%d %H:%M:%S", tm ) == 0 )
        snprintf( out, size, "%s", "?" );
}


static long long copy_to_stdout( int fd )
{
    char      buffer[ BUFFER_SIZE ];
    long long total = 0;

    for ( ;; )
    {
        ssize_t n = read( fd, buffer, sizeof( buffer ) );

        if ( n == 0 )
            break;

        if ( n < 0 )
        {
            if ( errno == EINTR )
                continue;

            return -1;
        }

        if ( fwrite( buffer, 1, (size_t) n, stdout ) != (size_t) n )
            return -1;

        total += n;
    }

    fflush( stdout );

    return total;
}

static void usage( const char *program )
{
    fprintf( stderr, "Pouziti: %s [-a] soubor\n", program );
    fprintf( stderr, "   bez prepinace   sleduje jen nove pribyle radky (jako tail -f)\n" );
    fprintf( stderr, "   -a              vypise nejdriv cely stavajici obsah souboru\n" );
}

int main( int argc, char **argv )
{
    int from_start = 0;
    int c;

    opterr = 0;

    while ( ( c = getopt( argc, argv, "a" ) ) != -1 )
    {
        if ( c != 'a' )
        {
            fprintf( stderr, "Neznamy prepinac -%c\n\n", optopt );
            usage( argv[ 0 ] );
            return 1;
        }

        from_start = 1;
    }

    if ( optind != argc - 1 )
    {
        fprintf( stderr, "Chyba: ocekavam presne jedno jmeno souboru.\n\n" );
        usage( argv[ 0 ] );
        return 1;
    }

    const char *name = argv[ optind ];

    struct sigaction sa;

    sa.sa_handler = on_signal;
    sigemptyset( &sa.sa_mask );
    sa.sa_flags = 0;

    sigaction( SIGINT,  &sa, NULL );
    sigaction( SIGTERM, &sa, NULL );

    // Soubor otevreme jen jednou a po celou dobu behu ho drzime otevreny.
    int fd = open( name, O_RDONLY );

    if ( fd < 0 )
    {
        perror( name );
        return 2;
    }

    struct stat st;

    if ( fstat( fd, &st ) != 0 )
    {
        perror( "fstat" );
        return 2;
    }

    // Vychozi chovani je stejne jako u tail -f: zacneme az na konci souboru.
    off_t pos = from_start ? 0 : st.st_size;

    if ( lseek( fd, pos, SEEK_SET ) == (off_t) -1 )
    {
        perror( "lseek" );
        return 2;
    }

    char stamp[ 32 ];
    now_string( stamp, sizeof( stamp ) );

    fprintf( stderr, "[%s] sleduji soubor %s, velikost %lld B%s\n",
             stamp, name, (long long) st.st_size,
             from_start ? ", vypisuji od zacatku" : "" );
    fprintf( stderr, "Ukonceni pomoci Ctrl-C.\n" );

    off_t     last_size = st.st_size;
    long long total     = 0;        // kolik bajtu jsme celkem vypsali
    int       unlinked  = 0;        // uz jsme hlasili, ze soubor byl smazan?

    // Pri -a vypiseme rovnou cely stavajici obsah.
    if ( from_start && st.st_size > 0 )
    {
        long long n = copy_to_stdout( fd );

        if ( n < 0 )
        {
            perror( "read" );
            return 2;
        }

        pos   += n;
        total += n;
    }

    while ( running )
    {
        sleep( POLL_SECONDS );      // kdyz spanek prerusi signal, nevadi

        if ( !running )
            break;

        // Stav souboru zjistujeme pres otevreny deskriptor, ne pres jmeno.
        if ( fstat( fd, &st ) != 0 )
        {
            perror( "fstat" );
            break;
        }

        // Kdyz nekdo soubor smaze, deskriptor zustava platny a data jsou
        // dal dostupna - i-uzel se uvolni az po zavreni posledniho odkazu.
        if ( st.st_nlink == 0 && !unlinked )
        {
            now_string( stamp, sizeof( stamp ) );
            fprintf( stderr, "[%s] soubor byl smazan, ale zustava otevreny - ctu dal.\n", stamp );
            unlinked = 1;
        }

        if ( st.st_size != last_size )
        {
            now_string( stamp, sizeof( stamp ) );

            if ( st.st_size < last_size )
                fprintf( stderr, "[%s] soubor zkracen: %lld -> %lld B (%lld)\n",
                         stamp, (long long) last_size, (long long) st.st_size,
                         (long long) ( st.st_size - last_size ) );
            else
                fprintf( stderr, "[%s] velikost: %lld -> %lld B (+%lld)\n",
                         stamp, (long long) last_size, (long long) st.st_size,
                         (long long) ( st.st_size - last_size ) );

            last_size = st.st_size;
        }

        // Soubor je kratsi, nez kam jsme se docetli - byl zkracen.
        // Vratime se lseek() na zacatek a cteme znovu.
        if ( st.st_size < pos )
        {
            now_string( stamp, sizeof( stamp ) );
            fprintf( stderr, "[%s] ctu znovu od zacatku souboru.\n", stamp );

            if ( lseek( fd, 0, SEEK_SET ) == (off_t) -1 )
            {
                perror( "lseek" );
                break;
            }

            pos = 0;
        }

        if ( st.st_size > pos )
        {
            long long n = copy_to_stdout( fd );

            if ( n < 0 )
            {
                perror( "read" );
                break;
            }

            pos   += n;
            total += n;
        }
    }

    fflush( stdout );

    now_string( stamp, sizeof( stamp ) );
    fprintf( stderr, "\n[%s] konec, celkem vypsano %lld B.\n", stamp, total );

    // Soubor zavirame az uplne na konci behu programu.
    close( fd );

    return 0;
}
