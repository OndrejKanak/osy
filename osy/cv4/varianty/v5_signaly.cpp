// ============================================================================
// v5_signaly.cpp - varianta 5: signaly mezi procesy
//
// Pouziti:  ./v5_signaly M N
//
//     M ... kolik datumu se vygeneruje; 0 = generuje porad, dokud Ctrl-C
//     N ... kolik datumu za sekundu
//
// Zapojeni je stejne jako v kalendar3 (3 potomci, 2 roury), navic:
//
//   1) Ctrl-C ukonci program KOREKTNE - nic se neztrati.
//      Terminal posle SIGINT cele skupine procesu (rodici i potomkum).
//      Potomci SIGINT ignoruji. Rodic ho zachyti a posle SIGTERM jen
//      generatoru. Generator dopise rozpracovany radek, zavre rouru A
//      a skonci. Potomek 2 tim dostane konec roury A, zpracuje zbytek,
//      zavre rouru B, potomek 3 dostane konec roury B a skonci taky.
//      Roury se vyprazdni "zepredu dozadu" a zadne datum se neztrati.
//
//   2) Potomek 3 posila rodici SIGUSR1 po kazdych 5 vypsanych radcich.
//      Rodic na nej reaguje hlaskou na stderr.
//
// Rucni test zvenku (v druhem terminalu):
//     kill -USR1 <pid rodice>        rodic vypise hlasku
//     kill -INT  <pid rodice>        totez co Ctrl-C
//
// Predmet: Operacni systemy
// ============================================================================

#include <time.h>
#include <signal.h>
#include <sys/wait.h>

#include "spolecne.hpp"

#define HLASIT_PO  5      // po kolika radcich posle potomek 3 rodici SIGUSR1

static volatile sig_atomic_t g_generuj   = 1;    // generator: bezet dal?
static volatile sig_atomic_t g_hlaseni   = 0;    // rodic: pocet SIGUSR1
static volatile pid_t        g_generator = 0;    // rodic: komu poslat SIGTERM

// ---------------------------------------------------------------------------
// Obsluhy signalu. V obsluze se smi volat jen "async-signal-safe" funkce -
// kill() a write() ano, printf() ne (mohl by byt zrovna rozpracovany).
// ---------------------------------------------------------------------------

// Generator: SIGTERM jen nastavi priznak, smycka ho pri dalsim kole uvidi.
static void generator_sigterm( int sig )
{
    (void) sig;
    g_generuj = 0;
}

// Rodic: SIGINT (Ctrl-C) preposle generatoru jako SIGTERM.
static void rodic_sigint( int sig )
{
    (void) sig;

    const char zprava[] = "\nrodic: Ctrl-C - posilam SIGTERM generatoru\n";
    write( STDERR_FILENO, zprava, sizeof( zprava ) - 1 );

    if ( g_generator > 0 )
        kill( g_generator, SIGTERM );
}

// Rodic: SIGUSR1 od potomka 3.
static void rodic_sigusr1( int sig )
{
    (void) sig;
    g_hlaseni++;

    const char zprava[] = "rodic: SIGUSR1 - potomek 3 hlasi dalsich 5 radku\n";
    write( STDERR_FILENO, zprava, sizeof( zprava ) - 1 );
}

static void nastav_signal( int sig, void ( *obsluha )( int ) )
{
    struct sigaction sa;

    memset( &sa, 0, sizeof( sa ) );
    sa.sa_handler = obsluha;
    sigemptyset( &sa.sa_mask );

    // SA_RESTART: kdyz signal prerusi systemove volani (read, waitpid...),
    // jadro ho samo zopakuje, misto aby vratilo chybu EINTR.
    sa.sa_flags = SA_RESTART;

    sigaction( sig, &sa, NULL );
}

int main( int argc, char **argv )
{
    if ( argc != 3 )
    {
        fprintf( stderr, "Pouziti: %s M N   (M = 0: bez konce, ukonceni Ctrl-C)\n", argv[ 0 ] );
        return 1;
    }

    int pocet      = nacti_cislo( argv[ 1 ], 0, "M" );
    int za_sekundu = nacti_cislo( argv[ 2 ], 1, "N" );

    fprintf( stderr, "rodic: PID %d  (zkus v jinem terminalu: kill -USR1 %d)\n",
             (int) getpid(), (int) getpid() );

    int roura_a[ 2 ];
    int roura_b[ 2 ];

    if ( pipe( roura_a ) < 0 || pipe( roura_b ) < 0 )
    {
        perror( "pipe" );
        return 2;
    }

    // Pred fork() nastavime ignorovani SIGINT - potomci ho zdedi, takze je
    // Ctrl-C z terminalu primo neukonci. Rodic si pak nastavi vlastni obsluhu.
    signal( SIGINT, SIG_IGN );

    // ===================== POTOMEK 1: generator ============================
    pid_t p1 = fork();

    if ( p1 == 0 )
    {
        close( roura_a[ 0 ] );
        close( roura_b[ 0 ] );
        close( roura_b[ 1 ] );

        nastav_signal( SIGTERM, generator_sigterm );
        srand( (unsigned) time( NULL ) ^ (unsigned) getpid() );

        int odeslano = 0;

        // pocet == 0 znamena "bez konce" - pak rozhoduje jen priznak.
        while ( g_generuj && ( pocet == 0 || odeslano < pocet ) )
        {
            char datum[ 16 ], radek[ 20 ];

            nahodne_datum( datum, sizeof( datum ) );
            snprintf( radek, sizeof( radek ), "%s\n", datum );

            if ( zapis_vse( roura_a[ 1 ], radek ) != 0 )
                break;

            odeslano++;
            usleep( 1000000 / za_sekundu );     // SIGTERM spanek prerusi
        }

        fprintf( stderr, "potomek 1: %s, odeslano %d datumu\n",
                 g_generuj ? "hotovo" : "dostal SIGTERM", odeslano );

        close( roura_a[ 1 ] );
        exit( 0 );
    }

    // ===================== POTOMEK 2: doplni jmena =========================
    pid_t p2 = fork();

    if ( p2 == 0 )
    {
        close( roura_a[ 1 ] );
        close( roura_b[ 0 ] );

        FILE *in = fdopen( roura_a[ 0 ], "r" );
        char  radek[ 64 ], vystup[ 128 ];

        while ( in != NULL && fgets( radek, sizeof( radek ), in ) != NULL )
        {
            doplnit_jmeno( radek, vystup, sizeof( vystup ) - 1 );
            strcat( vystup, "\n" );

            if ( zapis_vse( roura_b[ 1 ], vystup ) != 0 )
                break;
        }

        close( roura_b[ 1 ] );
        exit( 0 );
    }

    // ===================== POTOMEK 3: cisla radku + hlaseni rodici =========
    pid_t p3 = fork();

    if ( p3 == 0 )
    {
        close( roura_a[ 0 ] );
        close( roura_a[ 1 ] );
        close( roura_b[ 1 ] );

        FILE *in = fdopen( roura_b[ 0 ], "r" );
        char  radek[ 128 ];
        int   cislo = 0;

        while ( in != NULL && fgets( radek, sizeof( radek ), in ) != NULL )
        {
            printf( "(%d) %s", ++cislo, radek );
            fflush( stdout );

            if ( cislo % HLASIT_PO == 0 )
                kill( getppid(), SIGUSR1 );     // getppid() = PID rodice
        }

        fprintf( stderr, "potomek 3: konec roury B, vypsano %d radku\n", cislo );
        exit( 0 );
    }

    // ===================== RODIC ===========================================
    g_generator = p1;
    nastav_signal( SIGINT,  rodic_sigint );
    nastav_signal( SIGUSR1, rodic_sigusr1 );

    close( roura_a[ 0 ] );
    close( roura_a[ 1 ] );
    close( roura_b[ 0 ] );
    close( roura_b[ 1 ] );

    pid_t potomci[ 3 ] = { p1, p2, p3 };

    for ( int i = 0; i < 3; i++ )
        if ( potomci[ i ] > 0 )
            waitpid( potomci[ i ], NULL, 0 );   // SA_RESTART: signal waitpid neprerusi

    fprintf( stderr, "rodic: vsichni potomci skoncili, prijato %d hlaseni SIGUSR1\n",
             (int) g_hlaseni );

    return 0;
}
