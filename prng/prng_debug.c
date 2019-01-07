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

static prng_t *prng_debug_init( prng_t *P, const uint32_t seed )
{
    P->seq = seed;
    return P;
}

static void prng_debug_fini( prng_t *P )
{
}

/* Create and destroy a number of PRNG objects */
static prng_t *prng_debug_create_array( const unsigned nobj, const uint32_t seed )
{
    prng_t *P = malloc( nobj * sizeof(struct prng_s) );
    if( P != NULL )
    {
        for( unsigned i=0; i < nobj; i++ )
        {
            prng_debug_init( P+i, seed );
        }
    }
    return P;
}

static void prng_debug_destroy_array( prng_t *P )
{
    free( P );
}

/* Create and initialise a single PRNG object. */
/* This is treated as a special case of allocating mutiple objects */
static prng_t *prng_debug_create( const uint32_t seed )
{
    return prng_create_array( 1, seed );
}

/* Create and initialise a single PRNG object. */
/* This is treated as a special case of allocating mutiple objects */
static void prng_debug_destroy( prng_t *P )
{
    prng_destroy_array( P );
}

/* Get the next pseudo-random number in the sequence */
static uint32_t prng_debug_get( prng_t *P )
{
    return P->seq++;
}

/* Retrieve the next pseudo-random number without advancing the sequence */
static uint32_t prng_debug_peek( prng_t *P )
{
    return P->seq;
}


/* PRNG methods for this implementation */
prng_driver_t prng_debug = 
{
    .prng_create = prng_debug_create,
    .prng_destroy = prng_debug_destroy,
    .prng_init = prng_debug_init,
    .prng_fini = prng_debug_fini,
    .prng_create_array = prng_debug_create_array,
    .prng_destroy_array = prng_debug_destroy_array,
    .prng_get = prng_debug_get,
    .prng_peek = prng_debug_peek,
};
