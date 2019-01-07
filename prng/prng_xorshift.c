/*------------------------------------------------------------------------------------------------*/
/* Pseudo-random number generator:
 * Generate useful random number sequences that are repeatable based on a seed value.
/* Begun 2018-2019, StackHPC Ltd */

#include <stdint.h>
#include <stdlib.h>

#include "prng.h"
#include "prng_priv.h"

/* 
 * Xor WOW algorithm - Based on Marsaglia's Xorshift RNGs (http://www.jstatsoft.org/v08/i14/paper) 
 * This is the default RNG used in the CUDA Toolkit
 */

struct prng_s
{
    uint32_t state[5];
    uint32_t current;
};

/*
 * Generate initial state for algorithm based on single seed value.
 */
static prng_t *prng_xorshift_init( prng_t *P, const uint32_t seed )
{
    if (P != NULL)
    {
        uint32_t s = seed;
        P->current = s;
        s = P->state[0] = (s<<6|s>>26);
        s = P->state[1] = (s<<6|s>>26);
        s = P->state[2] = (s<<6|s>>26);
        s = P->state[3] = (s<<6|s>>26);
        P->state[4] = (s<<6|s>>26);
    }
    return P;
}

static void prng_xorshift_fini( prng_t *P )
{
}

/* Create and destroy a single PRNG object */
static prng_t *prng_xorshift_create( const uint32_t seed )
{
    return prng_xorshift_init( (prng_t *)malloc( sizeof(struct prng_s) ), seed);
}

static void prng_xorshift_destroy( prng_t *P )
{
    free( P );
}

/* Get the next pseudo-random number in the sequence */

/*
 * This function implements the 'xorwow' algorithm from
 * the Marsaglia paper (p5). This is the default Nvidia CUDA
 * toolkit RNG.
 */
static uint32_t prng_xorshift_next( prng_t *P )
{
    uint32_t s, t;

    s = P->state[0];

    /* XOR */
    t = P->state[3];
    t ^= t >> 2;
    t ^= t << 1;

    /* Shuffle */
    P->state[3] = P->state[2];
    P->state[2] = P->state[1];
    P->state[1] = s;

    /* XOR Phase 2 */
    t ^= s;
    t ^= s << 4;

    P->state[0] = t;

    P->current = t + (P->state[4] += 32437);
    return P->current;
}

/* Retrieve the current pseudo-random number without advancing the sequence */
static uint32_t prng_xorshift_peek( prng_t *P )
{
    return P->current;
}


/* PRNG methods for this implementation */
prng_driver_t prng_xorshift = 
{
    .prng_create = prng_xorshift_create,
    .prng_destroy = prng_xorshift_destroy,
    .prng_init = prng_xorshift_init,
    .prng_fini = prng_xorshift_fini,
    .prng_next = prng_xorshift_next,
    .prng_peek = prng_xorshift_peek,
};
