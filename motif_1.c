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
    time_now( &time_start );
    prng_select( PRNG_DEBUG );
    sample_select( SAMPLE_DEBUG );
    storage_select( STORAGE_DEBUG );
    const int result = storage_create( STORAGE_WORKSPACE );
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
