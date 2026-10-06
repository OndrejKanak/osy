// ============================================================================
// myls.cpp - ls + stat dohromady: opakovany vypis sledovanych souboru
//
// Pouziti:  ./myls [-s] [-t] [-r] [-S | -N] [-u] soubor ... 2>/dev/pts/X
//
//   Sloupce (vypisuji se v poradi, v jakem jsou prepinace zadany):
//     -s   velikost souboru v bajtech
//     -t   cas posledni zmeny obsahu (st_mtime)
//     -r   pristupova prava ve tvaru  -rw-r--r--
//
//   Trideni (zadava se pri spusteni, plati pro kazdy vypis):
//     -S   podle velikosti (od nejmensiho)
//     -N   podle jmena (abecedne)
//     -u   obraceny smer trideni
//     bez -S/-N zustava poradi, v jakem byly soubory zadany
//
// Program vypisuje soubory stale dokola, 1x za POLL_SECONDS sekund, dokud
// ho neukoncime Ctrl-C.
//
//   - soubor, ktery zmizel, ma misto vsech udaju jen otazniky
//   - soubor, ktery nejde cist, je ve vypisu oznaceny  <-- NELZE CIST
//   - kdyz se zmeni velikost souboru, vypise se na stderr radek
//         ----- soubor
//     a za nim JEN NOVA DATA (od puvodni do nove velikosti)
//
// Soubory se NEDRZI otevrene. Mezi vypisy si pamatujeme jen jmeno a posledni
// znamou velikost; soubor se otevre jen pri zmene velikosti, prectou se nova
// data a hned se zase zavre.
//
// stderr je dobre presmerovat do druheho terminalu (jeho jmeno ukaze  tty):
//     ./myls -S -s -r *.txt 2>/dev/pts/3
//
// Predmet: Operacni systemy
// ============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include <unistd.h>       // getopt, access, read, write, close, lseek
#include <fcntl.h>        // open
#include <sys/stat.h>     // stat, struct stat, makra S_IS* a S_IR*
#include <time.h>         // localtime, strftime

#define MAX_OPTIONS    32   // rozumny strop na pocet zadanych sloupcu
#define POLL_SECONDS    2   // jak casto se vypis opakuje
#define BUFFER_SIZE  4096

// Jak tridit.
enum sort_mode { SORT_NONE, SORT_NAME, SORT_SIZE };

// Stav jednoho sledovaneho souboru. Pole techto struktur se kazde kolo
// setridi, stav se tedy stehuje spolu se souborem.
struct entry
{
    const char *name;       // ukazatel do argv, zije po celou dobu behu
    int         order;      // poradi na prikazove radce (pro SORT_NONE)

    int         exists;     // podarilo se v tomto kole stat()?
    int         readable;   // ma proces pravo soubor cist?
    struct stat st;         // vysledek stat() z tohoto kola

    off_t       known;      // velikost, do ktere uz jsme data "videli"
};

static enum sort_mode sort_by  = SORT_NONE;
static int            reversed = 0;

static volatile sig_atomic_t running = 1;

static void on_signal( int sig )
{
    (void) sig;
    running = 0;
}

// ---------------------------------------------------------------------------
// Prevede prava ze st_mode na retezec ve tvaru "-rwxr-xr-x" (10 znaku + '\0').
// Beze zmeny prevzato z pripravy (cv2).
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
// Prevede cas na retezec "RRRR-MM-DD HH:MM:SS" (19 znaku).
// ---------------------------------------------------------------------------
static void format_time( time_t when, char *out, size_t size )
{
    struct tm *tm = localtime( &when );

    if ( tm == NULL || strftime( out, size, "%Y-%m-%d %H:%M:%S", tm ) == 0 )
        snprintf( out, size, "%s", "?" );
}

// ---------------------------------------------------------------------------
// Porovnani dvou polozek pro qsort().
//
// Soubory, ktere neexistuji, jsou vzdy na konci (o jejich velikosti nic
// nevime). Mezi sebou se pak radi podle jmena, aby vypis neposkakoval.
// Stejne velke soubory se pri trideni podle velikosti radi podle jmena.
// ---------------------------------------------------------------------------
static int compare_entries( const void *a, const void *b )
{
    const struct entry *x = (const struct entry *) a;
    const struct entry *y = (const struct entry *) b;

    if ( x->exists != y->exists )
        return x->exists ? -1 : 1;

    int result = 0;

    switch ( sort_by )
    {
        case SORT_SIZE:
            if ( x->exists && x->st.st_size != y->st.st_size )
                result = ( x->st.st_size < y->st.st_size ) ? -1 : 1;
            else
                result = strcmp( x->name, y->name );
            break;

        case SORT_NAME:
            result = strcmp( x->name, y->name );
            break;

        case SORT_NONE:
            result = x->order - y->order;
            break;
    }

    // Obraceny smer se tyka jen existujicich souboru - chybejici zustanou
    // porad na konci (ten pripad uz byl vyrizen nahore).
    return reversed ? -result : result;
}

// ---------------------------------------------------------------------------
// Zapise cely buffer na stderr. write() muze zapsat mene, nez se chtelo,
// proto se to opakuje ve smycce.
// ---------------------------------------------------------------------------
static int write_all( int fd, const char *buffer, size_t length )
{
    while ( length > 0 )
    {
        ssize_t n = write( fd, buffer, length );

        if ( n < 0 )
        {
            if ( errno == EINTR )
                continue;

            return -1;
        }

        buffer += n;
        length -= (size_t) n;
    }

    return 0;
}

// ---------------------------------------------------------------------------
// Soubor zmenil velikost: vypise na stderr "----- jmeno" a za tim nova data
// z rozsahu <from, to). Soubor se otevre jen na tuto chvili a hned se zavre.
// ---------------------------------------------------------------------------
static void report_change( const char *name, off_t from, off_t to )
{
    // Pres stdio i write() jde na stejny deskriptor 2. stderr neni
    // bufferovany, takze se poradi vypisu nepomicha.
    fprintf( stderr, "----- %s\n", name );

    if ( to < from )
    {
        fprintf( stderr, "(soubor zkracen: %lld -> %lld B, zadna nova data)\n",
                 (long long) from, (long long) to );
        return;
    }

    int fd = open( name, O_RDONLY );

    if ( fd < 0 )
    {
        fprintf( stderr, "(nelze otevrit: %s)\n", strerror( errno ) );
        return;
    }

    if ( lseek( fd, from, SEEK_SET ) == (off_t) -1 )
    {
        fprintf( stderr, "(lseek selhal: %s)\n", strerror( errno ) );
        close( fd );
        return;
    }

    char  buffer[ BUFFER_SIZE ];
    off_t remaining = to - from;    // cteme jen do velikosti zjistene stat()
    char  last      = '\n';

    while ( remaining > 0 )
    {
        size_t  want = remaining < (off_t) sizeof( buffer ) ? (size_t) remaining
                                                            : sizeof( buffer );
        ssize_t n    = read( fd, buffer, want );

        if ( n < 0 )
        {
            if ( errno == EINTR )
                continue;

            fprintf( stderr, "(chyba cteni: %s)\n", strerror( errno ) );
            break;
        }

        if ( n == 0 )               // soubor se mezitim zkratil
            break;

        if ( write_all( STDERR_FILENO, buffer, (size_t) n ) != 0 )
            break;

        last       = buffer[ n - 1 ];
        remaining -= n;
    }

    // Kdyz nova data nekonci odradkovanim, doplnime ho, aby dalsi
    // "----- jmeno" zacinalo na novem radku.
    if ( last != '\n' )
        fputc( '\n', stderr );

    close( fd );
}

// ---------------------------------------------------------------------------
// Zjisti aktualni stav souboru a pripadne ohlasi zmenu velikosti.
// ---------------------------------------------------------------------------
static void refresh_entry( struct entry *e, int first_round )
{
    // stat(), ne lstat(): sledujeme obsah souboru, a ten se cte pres
    // symbolicky odkaz stejne jako pres puvodni jmeno.
    e->exists = ( stat( e->name, &e->st ) == 0 );

    if ( !e->exists )
    {
        // Soubor zmizel. Kdyz se znovu objevi, bude cely jeho obsah novy.
        e->known    = 0;
        e->readable = 0;
        return;
    }

    // access() se pta s realnym UID/GID procesu - odpovida tedy presne tomu,
    // jestli by open() pro cteni uspel. (Pozor: root smi cist vse.)
    e->readable = ( access( e->name, R_OK ) == 0 );

    off_t size = e->st.st_size;

    // Pri prvnim vypisu si jen zapamatujeme vychozi velikost - "nova" data
    // jsou az ta, ktera pribudou behem sledovani.
    if ( first_round )
    {
        e->known = size;
        return;
    }

    if ( size == e->known )
        return;

    // Obsah cteme jen u obycejnych souboru. U adresare se sice velikost
    // taky muze zmenit, ale read() nad nim nedava smysl.
    if ( S_ISREG( e->st.st_mode ) )
        report_change( e->name, e->known, size );
    else
        fprintf( stderr, "----- %s\n(velikost %lld -> %lld B, neni obycejny soubor)\n",
                 e->name, (long long) e->known, (long long) size );

    e->known = size;
}

// ---------------------------------------------------------------------------
// Vypise jeden radek. U chybejiciho souboru jsou vsechny udaje otazniky.
// ---------------------------------------------------------------------------
static void print_entry( const struct entry *e, const char *options, int option_count )
{
    for ( int o = 0; o < option_count; o++ )
    {
        switch ( options[ o ] )
        {
            case 's':
                if ( e->exists )
                    printf( "%12lld ", (long long) e->st.st_size );
                else
                    printf( "%12s ", "?" );
                break;

            case 't':
            {
                char buffer[ 32 ];

                if ( e->exists )
                    format_time( e->st.st_mtime, buffer, sizeof( buffer ) );
                else    // "????-??-?? ??:??:??" nejde napsat primo - trigraf ??-
                    snprintf( buffer, sizeof( buffer ), "%s", "????" "-??" "-?? ??:??:??" );

                printf( "%-19s ", buffer );
                break;
            }

            case 'r':
            {
                char buffer[ 11 ];

                if ( e->exists )
                    format_mode( e->st.st_mode, buffer );
                else
                    snprintf( buffer, sizeof( buffer ), "%s", "??????????" );

                printf( "%-10s ", buffer );
                break;
            }
        }
    }

    // Jmeno je vzdy posledni sloupec vpravo, za nim pripadne poznamka.
    printf( "%s", e->name );

    if ( !e->exists )
        printf( "   <-- NENALEZEN" );
    else if ( !e->readable )
        printf( "   <-- NELZE CIST" );

    printf( "\n" );
}

static void usage( const char *program )
{
    fprintf( stderr, "Pouziti: %s [-s] [-t] [-r] [-S | -N] [-u] soubor ...\n", program );
    fprintf( stderr, "   -s   velikost souboru v bajtech\n" );
    fprintf( stderr, "   -t   cas posledni zmeny obsahu\n" );
    fprintf( stderr, "   -r   pristupova prava\n" );
    fprintf( stderr, "   -S   tridit podle velikosti\n" );
    fprintf( stderr, "   -N   tridit podle jmena\n" );
    fprintf( stderr, "   -u   obratit smer trideni\n" );
    fprintf( stderr, "Sloupce se vypisuji v poradi, v jakem jsou prepinace zadany.\n" );
    fprintf( stderr, "Vypis se opakuje kazde %d s, konec pomoci Ctrl-C.\n", POLL_SECONDS );
    fprintf( stderr, "Priklad: %s -S -u -s -r *.txt 2>/dev/pts/3\n", program );
}

int main( int argc, char **argv )
{
    char options[ MAX_OPTIONS ];    // sloupce v poradi, jak byly zadany
    int  option_count = 0;
    int  c;

    opterr = 0;     // chybove hlasky si vypiseme sami

    while ( ( c = getopt( argc, argv, "strSNu" ) ) != -1 )
    {
        switch ( c )
        {
            case 'S': sort_by  = SORT_SIZE; break;
            case 'N': sort_by  = SORT_NAME; break;
            case 'u': reversed = 1;         break;

            case 's':
            case 't':
            case 'r':
                if ( option_count >= MAX_OPTIONS )
                {
                    fprintf( stderr, "Chyba: prilis mnoho prepinacu (max %d).\n", MAX_OPTIONS );
                    return 1;
                }

                options[ option_count++ ] = (char) c;
                break;

            default:
                fprintf( stderr, "Neznamy prepinac -%c\n\n", optopt );
                usage( argv[ 0 ] );
                return 1;
        }
    }

    if ( optind >= argc )
    {
        fprintf( stderr, "Chyba: nezadan zadny soubor.\n\n" );
        usage( argv[ 0 ] );
        return 1;
    }

    int           count   = argc - optind;
    struct entry *entries = (struct entry *) calloc( (size_t) count, sizeof( struct entry ) );

    if ( entries == NULL )
    {
        fprintf( stderr, "Chyba: nedostatek pameti.\n" );
        return 2;
    }

    for ( int i = 0; i < count; i++ )
    {
        entries[ i ].name  = argv[ optind + i ];
        entries[ i ].order = i;
    }

    struct sigaction sa;

    sa.sa_handler = on_signal;
    sigemptyset( &sa.sa_mask );
    sa.sa_flags = 0;

    sigaction( SIGINT,  &sa, NULL );
    sigaction( SIGTERM, &sa, NULL );

    const char *sort_text = sort_by == SORT_SIZE ? "velikost"
                          : sort_by == SORT_NAME ? "jmeno"
                                                 : "poradi zadani";

    // Na terminalu obrazovku pred kazdym vypisem smazeme (jako watch),
    // pri presmerovani do souboru jen oddelime jednotlive vypisy.
    int clear_screen = isatty( STDOUT_FILENO );

    for ( int round = 0; running; round++ )
    {
        for ( int i = 0; i < count; i++ )
            refresh_entry( &entries[ i ], round == 0 );

        qsort( entries, (size_t) count, sizeof( struct entry ), compare_entries );

        char stamp[ 32 ];
        format_time( time( NULL ), stamp, sizeof( stamp ) );

        if ( clear_screen )
            printf( "\033[H\033[2J" );
        else if ( round > 0 )
            printf( "\n" );

        printf( "=== %s  trideni: %s%s  (Ctrl-C = konec)\n",
                stamp, sort_text, reversed ? ", obracene" : "" );

        for ( int i = 0; i < count; i++ )
            print_entry( &entries[ i ], options, option_count );

        // Pri presmerovani do souboru je stdout plne bufferovany.
        fflush( stdout );

        sleep( POLL_SECONDS );      // kdyz spanek prerusi signal, nevadi
    }

    printf( "\nkonec.\n" );

    free( entries );

    return 0;
}
