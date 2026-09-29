// ============================================================================
// gennum.cpp - generator cisel, ktery bezi, dokud ho neukoncime <Ctrl-C>
//
// Pouziti:  ./gennum M N
//           M = MAXIMALNI pocet cisel na radku (skutecny pocet je nahodny 1..M)
//           N = pocet radku za minutu
//
// Priklad:  ./gennum 10 60 > out.txt     ... zhruba jeden radek za sekundu
//           ./gennum 20 12 > out.txt     ... jeden radek za pet sekund
//
// V druhem terminalu se da narustani souboru sledovat:
//           tail -f out.txt
//           ../monitor/monitor out.txt
//
// DULEZITE: po kazdem radku se vola fflush( stdout ). Kdyz vystup nejde na
// terminal, ale do souboru, je stdout plne bufferovany (cca 4 kB) a bez
// fflush() by se do souboru dlouho nic nezapsalo - tail -f ani monitor by
// nemely co ukazovat. Na terminalu tento problem nenastane, protoze tam je
// stdout bufferovany po radcich.
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

// Rozsah generovanych cisel (obe meze vcetne) - zamerne nikdy ne jednociferna.
#define NUM_MIN   10
#define NUM_MAX 1000

// Priznak, ze prisel signal a ma se skoncit.
// V obsluze signalu se smi menit jen promenna typu volatile sig_atomic_t -
// jen u ni je zaruceno, ze se zapise jednou nedelitelnou operaci.
static volatile sig_atomic_t running = 1;

static void on_signal( int sig )
{
    (void) sig;     // parametr nepotrebujeme, jen umlcime prekladac
    running = 0;
}

// Prevede textovy argument na kladne cele cislo, jinak program ukonci.
static long parse_positive( const char *text, const char *name )
{
    char *end = NULL;

    errno = 0;
    long value = strtol( text, &end, 10 );

    if ( errno != 0 || end == text || *end != '\0' || value <= 0 )
    {
        fprintf( stderr, "Chyba: argument %s musi byt kladne cele cislo, dostal jsem '%s'.\n",
                 name, text );
        exit( 1 );
    }

    return value;
}

int main( int argc, char **argv )
{
    if ( argc != 3 )
    {
        fprintf( stderr, "Pouziti: %s M N\n", argv[ 0 ] );
        fprintf( stderr, "   M = maximalni pocet cisel na radku (skutecny pocet je nahodny 1 az M)\n" );
        fprintf( stderr, "   N = pocet radku za minutu\n" );
        fprintf( stderr, "Program bezi, dokud ho neukoncite pomoci Ctrl-C.\n" );
        fprintf( stderr, "Priklad: %s 10 60 > out.txt\n", argv[ 0 ] );
        return 1;
    }

    long max_numbers     = parse_positive( argv[ 1 ], "M" );
    long lines_per_minute = parse_positive( argv[ 2 ], "N" );

    // Obsluha Ctrl-C (SIGINT) a signalu TERM, aby se dal program ukoncit
    // ciste - dopsat rozdelany radek a vypsat souhrn.
    // sigaction je proti starsimu signal() prenositelnejsi a lepe definovana.
    struct sigaction sa;

    sa.sa_handler = on_signal;
    sigemptyset( &sa.sa_mask );
    sa.sa_flags = 0;            // zamerne BEZ SA_RESTART, aby se spanek prerusil

    sigaction( SIGINT,  &sa, NULL );
    sigaction( SIGTERM, &sa, NULL );

    srand( (unsigned int) time( NULL ) ^ ( (unsigned int) getpid() << 16 ) );

    // Rozlozeni intervalu mezi radky na sekundy a nanosekundy.
    double interval  = 60.0 / (double) lines_per_minute;
    time_t int_sec   = (time_t) interval;
    long   int_nsec  = (long) ( ( interval - (double) int_sec ) * 1e9 );

    fprintf( stderr, "Generuji %ld radku za minutu (jeden radek kazdych %.3f s), "
                     "max %ld cisel na radku.\n", lines_per_minute, interval, max_numbers );
    fprintf( stderr, "Ukonceni pomoci Ctrl-C.\n" );

    // Cas dalsiho radku pocitame absolutne od startu. Kdybychom po kazdem
    // radku jen "spali interval", postupne by se nascitavala chyba o dobu,
    // kterou trva samotny vypis.
    struct timespec next;
    clock_gettime( CLOCK_MONOTONIC, &next );

    long lines = 0;

    while ( running )
    {
        long count = 1 + rand() % max_numbers;      // nahodny pocet cisel 1..M

        for ( long i = 0; i < count; i++ )
            printf( "%s%d", i ? " " : "", NUM_MIN + rand() % ( NUM_MAX - NUM_MIN + 1 ) );

        printf( "\n" );
        fflush( stdout );       // bez tohoto by tail -f dlouho nic nevidel

        lines++;

        // Posuneme cas dalsiho radku o jeden interval.
        next.tv_sec  += int_sec;
        next.tv_nsec += int_nsec;

        if ( next.tv_nsec >= 1000000000L )
        {
            next.tv_nsec -= 1000000000L;
            next.tv_sec++;
        }

        // Spanek do presneho okamziku. clock_nanosleep() vraci chybovy kod
        // primo jako navratovou hodnotu, nikoli pres errno.
        while ( running )
        {
            int rc = clock_nanosleep( CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL );

            if ( rc == 0 )          // dospano az do pozadovaneho okamziku
                break;

            if ( rc != EINTR )      // jina chyba nez preruseni signalem
            {
                fprintf( stderr, "Chyba: clock_nanosleep selhal (%s).\n", strerror( rc ) );
                running = 0;
                break;
            }

            // rc == EINTR: prisel signal. Kdyz mame bezet dal, dospime zbytek
            // intervalu - cil je absolutni, takze se staci vratit do smycky.
        }
    }

    fflush( stdout );
    fprintf( stderr, "\nUkonceno signalem, vygenerovano %ld radku.\n", lines );

    return 0;
}
