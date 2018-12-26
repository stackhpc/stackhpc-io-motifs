/*------------------------------------------------------------------------------------------------*/
/* Pseudo-random number generator:
 * Generate useful random number sequences that are repeatable based on a seed value.
 * Support multiple concurrent random number sequences. */
/* Begun 2018-2019, StackHPC Ltd */

#include <stdint.h>
#include <stdlib.h>

#include "prng.h"
#include "prng_priv.h"

/* Reference implementation using incrementing sequences (ie, same API but no randomness) */

struct prng_s
{
    uint32_t seq;
};

/* Create and destroy a single PRNG object */
static prng_t *prng_debug_create( const uint32_t seed )
{
    prng_t *P = malloc( sizeof(struct prng_s) );
    if( P != NULL )
    {
            P->seq = seed;
    }
    return P;
}

static void prng_debug_destroy( prng_t *P )
{
    free( P );
}

/* Get the next pseudo-random number in the sequence */
static uint32_t prng_debug_random( prng_t *P )
{
    return P->seq++;
}


/* PRNG methods for this implementation */
prng_driver_t prng_debug = 
{
    .prng_create = prng_debug_create,
    .prng_destroy = prng_debug_destroy,
    .prng_random = prng_debug_random,
};
