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

#define OBJ_COUNT 1000
#define STORAGE_WORKSPACE "motif_1-data" 

int main( int argc, char *argv[] )
{
    uint32_t obj_id[OBJ_COUNT];
    struct timespec ts_write, ts_read, ts_delta;

    /* Application setup and early configuration */
    time_now( &time_start );
    prng_select( PRNG_DEBUG );
    sample_select( SAMPLE_DEBUG );
    storage_select( STORAGE_RADOS );
    const int result = storage_create( STORAGE_WORKSPACE, argc, argv );
    if( result < 0 )
    {
        return -1;
    }

    /* Synchronise and start the benchmark */
    time_now( &time_benchmark );

    /* Write out phase */

    /* Read back phase */

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

    time_now( &ts_write );
    time_delta( &time_benchmark, &ts_write, &ts_delta );
    const float writes_per_sec = (float)OBJ_COUNT / ((float)ts_delta.tv_sec + (float)ts_delta.tv_nsec / 1000000000.0);
    log_info( "Wrote %u objects in %ld.%03lds = %g objects/second", OBJ_COUNT,
            ts_delta.tv_sec, ts_delta.tv_nsec / 1000000l, writes_per_sec );

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

    time_now( &ts_read );
    time_delta( &ts_write, &ts_read, &ts_delta );
    const float reads_per_sec = (float)OBJ_COUNT / ((float)ts_delta.tv_sec + (float)ts_delta.tv_nsec / 1000000000.0);
    log_info( "Read %u objects in %ld.%03lds = %g objects/second", OBJ_COUNT,
            ts_delta.tv_sec, ts_delta.tv_nsec / 1000000l, reads_per_sec );

    storage_destroy( );
    return 0;
}
