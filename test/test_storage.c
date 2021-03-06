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
#include <unistd.h>

#include "prng.h"
#include "sample.h"
#include "storage.h"
#include "utils.h"

#define OBJ_COUNT 10
#define STORAGE_WORKSPACE "motif_1-data" 

int main( int argc, char *argv[] )
{
    uint32_t obj_id[OBJ_COUNT];

    /* Application setup and early configuration */
    prng_select( PRNG_DEBUG );
    sample_select( SAMPLE_DEBUG );
    storage_select( STORAGE_DEBUG );
    const int result = storage_create( STORAGE_WORKSPACE, argc, argv );
    if( result < 0 )
    {
        return -1;
    }

    const pid_t client_id = getpid(); 
    prng_t *P = prng_create( 42 );
    sample_t *S = sample_create( P );

    /* Write out phase */
    for( unsigned i=0; i < OBJ_COUNT; i++ )
    {
        obj_id[i] = prng_peek(P);
        sample_init( S, P );
        storage_write( client_id, obj_id[i], S );
    }

    /* Read back all objects */
    prng_init( P, 42 );
    for( unsigned i=0; i < OBJ_COUNT; i++ )
    {
        prng_init( P, obj_id[i] );
        storage_read( client_id, obj_id[i], S );
        if( !sample_valid( S, P ) )
        {
            log_error( "Object %d is not valid", i );
        }
    }

    storage_destroy( );
    return 0;
}
