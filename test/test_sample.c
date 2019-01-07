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

#include "prng.h"
#include "sample.h"
#include "utils.h"

int main( int argc, char *argv[] )
{
    /* Application setup and early configuration */
    prng_select( PRNG_DEBUG );
    sample_select( SAMPLE_DEBUG );

    prng_t *P = prng_create( 42 );
    sample_t *S = sample_create( P );
    const uint8_t *data = sample_data( S );
    const size_t len = sample_len( S );
    for( unsigned i=0; i < len; i++ )
    {
        printf( "%02X ", data[i] );
        if( i % 32 == 31 || i == len-1 )
            printf( "\n" );
    }
    prng_init( P, 42 );
    const bool valid = sample_valid( S, P );
    printf( "Sample is %s\n", valid ? "valid" : "INVALID" );
    sample_destroy( S );
    assert( valid );
    return 0;
}
