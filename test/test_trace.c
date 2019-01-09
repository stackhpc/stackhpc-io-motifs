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
#include <assert.h>

#include "utils.h"

int main( int argc, char *argv[] )
{
    struct timespec t_start, t_iter, t_delta;
    int i;

    time_now( &t_start );

    if (trace_init( ".", 0)) {
        log_error( "trace init failed" );
        exit(1);
    }

    for( i=0; i<500; i++ ) {
        time_now( &t_iter );
        time_delta( &t_start, &t_iter, &t_delta );
        trace_read( &t_delta, &t_delta );
        trace_write( &t_delta, &t_delta );
        trace( TRACE_MISC, &t_delta, &t_delta, "teststr" );
    }
    
    trace_fini();
    return 0;
}
