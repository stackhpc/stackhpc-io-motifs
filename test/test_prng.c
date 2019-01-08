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
    uint32_t value[5], newvalue[5];
    /* Application setup and early configuration */
    prng_select( PRNG_XORSHIFT );

    printf( "First seed sequence\n" );
    /* Test repeatability */
    prng_t *P = prng_create( 42 );

    for (int i = 0; i < 5; i++)
        printf( "Rand[%d] = %x\n", i, value[i]=prng_next(P));

    printf( "\nReset seed\n" );
    prng_init( P, 42 );

    for (int i = 0; i < 5; i++) {
        printf( "Rand[%d] = %x\n", i, newvalue[i]=prng_next(P));
        assert( value[i] == newvalue[i]);
    }

    /* Test seed uniqueness */
    printf( "\nNew seed\n" );
    prng_init( P, -42 );

    for (int i = 0; i < 5; i++) {
        printf( "Rand[%d] = %x\n", i, newvalue[i]=prng_next(P));
        assert( value[i] != newvalue[i]);
    }

    prng_destroy( P );
    return 0;
}
