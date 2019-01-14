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
    { "trace", 't', 0, 0, "Collect Traces" },
    { 0 }
};

struct motif_arguments
{
    sample_impl_t 	sample;
    prng_impl_t 	prng;
    storage_impl_t	storage;
    bool		trace;
    char		*workspace;
};

static int find_match ( char **list, char *which )
{
    int match = 0;
    while ( list[match] ) {
        if ( strcasecmp( list[match], which ) == 0 )
            return 0;
        ++match;
    }
    return -1;
}

static error_t parse_opt( int key, char *arg, struct argp_state *state )
{
    struct motif_arguments *motif_arguments = state->input;

    char *storage_impl_str[] = 	STORAGE_IMPL_STR;
    char *prng_impl_str[] = 	PRNG_IMPL_STR;
    char *sample_impl_str[] = 	SAMPLE_IMPL_STR;

    switch (key) {
    case 't':
        motif_arguments->trace = true;
        break;

    case 'r':
        if ( (motif_arguments->prng = find_match( prng_impl_str, arg )) )
            return ARGP_ERR_UNKNOWN;
        break;

    case 's':
        if ( (motif_arguments->sample = find_match( sample_impl_str, arg )) )
            return ARGP_ERR_UNKNOWN;
        break;
    
    case 'S':
        if ( (motif_arguments->storage = find_match( storage_impl_str, arg )) )
            return ARGP_ERR_UNKNOWN;
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

int main( int argc, char *argv[] )
{
    uint32_t obj_id[OBJ_COUNT];
    struct motif_arguments motif_arguments;

    /* Set default argument values */
    motif_arguments.sample = 	SAMPLE_DEBUG;
    motif_arguments.prng = 	PRNG_DEBUG;
    motif_arguments.storage = 	STORAGE_DEBUG;
    motif_arguments.workspace = STORAGE_WORKSPACE;
    motif_arguments.trace =	true;

    argp_parse( &argp, argc, argv, 0, 0, &motif_arguments );

    log_debug( "sample = %d\n", motif_arguments.sample );
    log_debug( "prng = %d\n", motif_arguments.prng );
    log_debug( "storage = %d\n", motif_arguments.storage );
    log_debug( "workspace = %s\n", motif_arguments.workspace );
    log_debug( "trace = %d\n", motif_arguments.trace );

    exit(1);

    /* Application setup and early configuration */
    time_now( &time_start );
    prng_select( motif_arguments.prng );
    sample_select( motif_arguments.sample );
    storage_select( motif_arguments.storage );
    const int result = storage_create( motif_arguments.workspace );
    if( result < 0 )
    {
        return -1;
    }

    /* Synchronise and start the benchmark */
    time_now( &time_benchmark );

    /* Write out phase */

    /* Read back phase */

    storage_destroy( );
    return 0;
}
