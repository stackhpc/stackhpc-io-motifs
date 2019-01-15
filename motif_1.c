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

#define OBJ_COUNT 10
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
    { "sample", 's', "SAMPLE", 0, "Sample type to use" },
    { "storage", 'S', "STORAGE", 0, "Storage type to use" } ,
    { "workspace", 'w', "WORKSPACE", 0, "Storage workspace to use" },
    { "tracedir", 't', "TRACEDIR", 0, "Directory for traces" },
    { "count", 'c', "OBJECT COUNT", 0, "Object count" },
    { "parallel", 'p', "TASK COUNT", 0, "Number of parallel tasks" },
    { 0 }
};

/* State structure for motif_1 command arguments */
struct motif_arguments
{
    sample_impl_t 	sample;		/* Sample used in test */
    prng_impl_t 	prng;		/* Random number generator */
    storage_impl_t	storage;	/* Storage selection */
    char		*trace_dir;	/* Directory for traces */
    char		*workspace;	/* Workspace pointer */
    int			object_count;	/* Number of objects */
    int			task_count;	/* Number of tasks */
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
    char options[PATH_MAX];

    switch (key) {
    case 'c':
        if ( (motif_arguments->object_count = atoi( arg )) <= 0 ) 
            argp_failure( state, 1, 0, "Count must be greater than 0" );
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

    case 't':
        motif_arguments->trace_dir = arg;
        break;

    case 'w':
        motif_arguments->workspace = arg;
        break;

    case ARGP_KEY_ARG: 
        return 0;

    default: 
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = { options, parse_opt, args_doc, prog_doc };
int run_motif( struct motif_arguments *map, const int ordinal );

int main( int argc, char *argv[] )
{
    uint32_t obj_id[OBJ_COUNT];
    struct motif_arguments motif_arguments;
    int status, ret;

    /* Set default argument values */
    motif_arguments.sample = 	 SAMPLE_DEBUG;
    motif_arguments.prng = 	 PRNG_DEBUG;
    motif_arguments.storage = 	 STORAGE_DEBUG;
    motif_arguments.workspace =  STORAGE_WORKSPACE;
    motif_arguments.trace_dir =	 ".";
    motif_arguments.task_count = 1;

    argp_parse( &argp, argc, argv, 0, 0, &motif_arguments );

    log_debug( "sample = %d\n", motif_arguments.sample );
    log_debug( "prng = %d\n", motif_arguments.prng );
    log_debug( "storage = %d\n", motif_arguments.storage );
    log_debug( "workspace = %s\n", motif_arguments.workspace );
    log_debug( "trace_dir = %s\n", motif_arguments.trace_dir );
    log_debug( "count = %d\n", motif_arguments.object_count );
    log_debug( "task_count = %d\n", motif_arguments.task_count );

    /* Spawn individual test tasks */
    for( int i=0; i < motif_arguments.task_count; i++ )
    {
        pid_t pid;
        if( (pid = fork()) == 0 ) {
            log_debug("in child");
            return( run_motif( &motif_arguments, i ) );
        }
        if ( pid < 0 )
        {
            log_error( "fork failed - %d\n", pid );
            return 1;
        }
    }

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
run_motif( struct motif_arguments *map, const int ordinal )
{
    log_debug( "child: ordinal %d", ordinal );

#ifdef TODO
    /* Application setup and early configuration */
    trace_init( map->trace_dir, ordinal );
    time_now( &time_start );
    prng_select( map->prng );
    sample_select( map->sample );
    storage_select( map->storage );
    const int result = storage_create( map->workspace );
    if( result < 0 )
    {
        return -1;
    }

    /* Synchronise and start the benchmark */
    time_now( &time_benchmark );

    /* Write out phase */

    /* Read back phase */

    storage_destroy( );
#endif
    return 0;
}
