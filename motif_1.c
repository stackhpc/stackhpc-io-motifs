/*--------------------------------------------------------------------------------------------*/
/* Storage benchmark motif 1: scattered small-file I/O
 * This motif aims to measure storage candidate performance for an
 * application workload with the following characteristics:
 * - Generate stimulus based on highly-concurrent access to a
 *   very large number of small files.
 * - Telemetry will be gathered for the factors that are likely to
 *   dominate overall performance.
 * - This scenario would adapt well to either file-based or object-based
 *   storage paradigms.
 *
 * Begun 2018-2019, StackHPC Ltd. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <argp.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>


#include "prng.h"
#include "sample.h"
#include "storage.h"
#include "utils.h"
#include "info.h"
#include "barrier.h"

#define STORAGE_WORKSPACE "motif_1-data" 

const char *argp_program_version = VERSION;
const char *argp_program_bug_address = SUPPORT_CONTACT;

static char args_doc[] = "";

static char prog_doc[] = 
"\nStorage benchmark motif 1: scattered small-file I/O\n\n\
This motif aims to measure storage candidate performance for an\n\
application workload with the following characteristics:\n\n\
 - Generate stimulus based on highly-concurrent access to a\n\
   very large number of small files.\n\
 - Telemetry will be gathered for the factors that are likely to\n\
   dominate overall performance.\n\
 - This scenario would adapt well to either file-based or object-based\n\
   storage paradigms.";

static struct argp_option options[] = 
{
    { "prng", 'r', "PRNG", 0, "Pseudo-random number generator to use" },
    { "seed", 'R', "SEED", 0, "Pseudo-random number generator seed" },
    { "sample", 's', "SAMPLE", 0, "Sample type to use" },
    { "storage", 'S', "STORAGE", 0, "Storage type to use" } ,
    { "workspace", 'W', "WORKSPACE", 0, "Storage workspace to use" },
    { "tracedir", 't', "TRACEDIR", 0, "Directory for traces" },
    { "write count", 'c', "OBJECT WRITE COUNT", 0, "Object write count" },
    { "read count", 'n', "OBJECT READ COUNT", 0, "Object read count" },
    { "parallel", 'p', "TASK COUNT", 0, "Number of parallel tasks" },
    { "verbose", 'v', "VERBOSITY", 0, "Verbosity level" },
    { 0 }
};

/* State structure for motif_1 command arguments */
struct motif_arguments
{
    sample_impl_t 	sample;		    /* Sample used in test */
    prng_impl_t 	prng;		    /* Random number generator */
    int 		seed;		    /* PRNG seed value */
    storage_impl_t	storage;	    /* Storage selection */
    log_level_t         verbosity;          /* Logging verbosity level */
    char		*trace_dir;	    /* Directory for traces */
    char		*workspace;	    /* Workspace pointer */
    unsigned		object_write_count; /* Number of objects */
    unsigned		object_read_count;  /* Number of objects */
    int			task_count;	    /* Number of tasks */
    char		**forward_argv;     /* Forward arguments (handled downstream) */
    int		        forward_argc;       /* Forward argument count */
};

/* Find matching ordinal for item in list */
static int find_match ( char **list, char *which )
{
    int match = 0;
    while ( list[match] ) {
        if ( strcasecmp( list[match], which ) == 0 ) {
            return match;
        }
        ++match;
    }
    return -1;
}

/* Utility function to create a string of options from string vector */
char *possible_options( char **option_list, char *buf )
{
    int opt = 0;
    buf[0] = '\0';
    while ( option_list[opt] ) {
        strcat( buf, option_list[opt++] );
        if ( option_list[opt] )
            strcat( buf, ", " );
    }
    return buf;
}

/* Handle command option parsing */
static error_t parse_opt( int key, char *arg, struct argp_state *state )
{
    struct motif_arguments *motif_arguments = state->input;

    char *storage_impl_str[] = 	STORAGE_IMPL_STR;
    char *prng_impl_str[] = 	PRNG_IMPL_STR;
    char *sample_impl_str[] = 	SAMPLE_IMPL_STR;
    char *log_level_str[] =     LOG_LEVEL_STR;
    char options[PATH_MAX];

    switch (key) {
    case 'c':
        if ( (motif_arguments->object_write_count = atoi( arg )) <= 0 ) 
            argp_failure( state, 1, 0, "Write count must be greater than 0" );
        break;

    case 'n':
        if ( (motif_arguments->object_read_count = atoi( arg )) <= 0 ) 
            argp_failure( state, 1, 0, "Read count must be greater than 0" );
        break;

    case 'p':
        if ( (motif_arguments->task_count = atoi( arg )) <= 0 ) 
            argp_failure( state, 1, 0, "Task count must be greater than 0" );
        break;

    case 'r':
        if ( (motif_arguments->prng = find_match( prng_impl_str, arg )) < 0 )
            argp_failure( state, 1, 0, 
                          "Pseudo-random number generator must be one of %s", 
                          possible_options( prng_impl_str, options ));
        break;

    case 'R':
        if ( (motif_arguments->seed = atoi( arg )) <= 0 ) 
            argp_failure( state, 1, 0, "Seed must be greater than 0" );
        break;

    case 's':
        if ( (motif_arguments->sample = find_match( sample_impl_str, arg ))  < 0)
            argp_failure( state, 1, 0, "Sample must be one of %s", 
                          possible_options( sample_impl_str, options ));
        break;
    
    case 'S':
        if ( (motif_arguments->storage = find_match( storage_impl_str, arg ))  < 0)
            argp_failure( state, 1, 0, "Storage must be one of %s", 
                          possible_options( storage_impl_str, options ));
        break;

    case 'v':
        if ( (motif_arguments->verbosity = find_match( log_level_str, arg ))  < 0)
            argp_failure( state, 1, 0, "Verbosity must be one of %s", 
                          possible_options( log_level_str, options ));
        break;

    case 't':
        motif_arguments->trace_dir = arg;
        break;

    case 'W':
        motif_arguments->workspace = arg;
        break;

    case ARGP_KEY_END: 
        return 0;

    case ARGP_KEY_ARG:
        motif_arguments->forward_argv[motif_arguments->forward_argc++] = arg;
        return 0;

    case ARGP_KEY_INIT:
        /* Set default argument values */
        motif_arguments->sample =	SAMPLE_DEBUG;
        motif_arguments->prng = 	PRNG_DEBUG;
        motif_arguments->storage = 	STORAGE_DEBUG;
        motif_arguments->verbosity =    LOG_DEBUG;
        motif_arguments->workspace = 	STORAGE_WORKSPACE;
        motif_arguments->task_count =	1;
        motif_arguments->trace_dir =	".";
        motif_arguments->forward_argv =	malloc( sizeof( char * ) * state->argc );
        motif_arguments->forward_argc = 0;
        break;

    default: 
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = { options, parse_opt, args_doc, prog_doc };
int run_motif( struct motif_arguments *map, barrier_t *bp, const int ordinal );

int main( int argc, char *argv[] )
{
    struct motif_arguments motif_arguments;
    int status, ret;
    barrier_t *bp;

    time_now( &time_start );
    argp_parse( &argp, argc, argv, 0, 0, &motif_arguments );

    log_set_level( motif_arguments.verbosity );

    log_debug( "Arguments:" );
    log_debug( "  sample = %d", motif_arguments.sample );
    log_debug( "  prng = %d", motif_arguments.prng );
    log_debug( "  storage = %d", motif_arguments.storage );
    log_debug( "  workspace = %s", motif_arguments.workspace );
    log_debug( "  trace_dir = %s", motif_arguments.trace_dir );
    log_debug( "  write count = %d", motif_arguments.object_write_count );
    log_debug( "  read count = %d", motif_arguments.object_read_count );
    log_debug( "  task_count = %d", motif_arguments.task_count );
    log_debug( "  seed = %d", motif_arguments.seed );

    log_debug( "  forward arguments:" );
    for( int i=0; i<motif_arguments.forward_argc; i++)
    {
        log_debug( "    %s", motif_arguments.forward_argv[i] );
    }

    sample_select( motif_arguments.sample );
    storage_select( motif_arguments.storage );

    bp = barrier_init( "/motif_1", motif_arguments.task_count + 1 );

    /* Spawn individual test tasks */
    for( int i=0; i < motif_arguments.task_count; i++ )
    {
        pid_t pid;
        if( (pid = fork()) == 0 ) {
            return( run_motif( &motif_arguments, bp, i ) );
        }
        if ( pid < 0 )
        {
            log_error( "fork failed - %d\n", pid );
            return 1;
        }
    }

    log_debug( "main waiting for barrier" );
    barrier_wait( bp );
    log_debug( "main passed barrier" );

    /* wait for tests to complete */
    while( (ret = wait( &status )) > 0 )
    {
        log_debug( "reaped child - %d", ret );
    }

    if( errno != ECHILD )
    {
        log_debug( "error in wait() - %d", errno );
        return 1;
    }
    return 0;
}

/*
 * Control function for execution of the requested motif. 
 */
int 
run_motif( struct motif_arguments *map, barrier_t *bp, const int ordinal )
{
    uint32_t *obj_id;
    struct timespec ts_read, ts_write, ts_delta;

    log_debug( "child: ordinal %d", ordinal );

    /* randomize each task seed unless explicitly set on init */
    if( map->seed == 0 ) 
    {
        struct timespec now;
        time_now( &now );
        map->seed = now.tv_sec ^ now.tv_nsec;
        log_debug( "updating local seed in %d to %d", ordinal, map->seed );
    }

    log_debug( "ord %d waiting for barrier", ordinal );
    barrier_wait( bp );
    log_debug( "ord %d passed barrier", ordinal );

    obj_id = malloc( map->object_write_count * sizeof(uint32_t) );
    if( obj_id == NULL )
    {
        log_error( "Could not alloc seed vector for %u objects", map->object_write_count );
        return -1;
    }

    /* Application setup and early configuration */
    trace_init( map->trace_dir, ordinal );
    prng_select( map->prng );
    prng_t *P = prng_create( map->seed );
    sample_t *S = sample_create( P );

    const int result = storage_create( map->workspace, map->forward_argc, map->forward_argv );
    if( result < 0 )
    {
        return -1;
    }

    /* Synchronise and start the benchmark */
    time_now( &time_benchmark );

    /* Write out phase */
    for( unsigned i=0; i < map->object_write_count; i++ )
    {
        obj_id[i] = prng_peek(P);
        sample_init( S, P );
        storage_write( ordinal, obj_id[i], S );
    }

    time_now( &ts_write );
    time_delta( &time_benchmark, &ts_write, &ts_delta );
    const float writes_per_sec = (float)map->object_write_count / ((float)ts_delta.tv_sec + (float)ts_delta.tv_nsec / 1000000000.0);
    log_info( "Wrote %u objects in %ld.%03lds = %g objects/second", map->object_write_count,
            ts_delta.tv_sec, ts_delta.tv_nsec / 1000000l, writes_per_sec );


    /* Read back phase */
    prng_init( P, map->seed );
    for( unsigned i=0; i < map->object_read_count; i++ )
    {
        const unsigned obj_idx = i % map->object_write_count;           /* FIXME: randomise selection? */
        prng_init( P, obj_id[obj_idx] );
        storage_read( ordinal, obj_id[obj_idx], S );
        if( !sample_valid( S, P ) )
        {
            log_error( "Object %d is not valid", obj_idx );
        }
    }

    time_now( &ts_read );
    time_delta( &ts_write, &ts_read, &ts_delta );
    const float reads_per_sec = (float)map->object_read_count / ((float)ts_delta.tv_sec + (float)ts_delta.tv_nsec / 1000000000.0);
    log_info( "Read %u objects in %ld.%03lds = %g objects/second", map->object_read_count,
            ts_delta.tv_sec, ts_delta.tv_nsec / 1000000l, reads_per_sec );

    storage_destroy( );
    return 0;
}
