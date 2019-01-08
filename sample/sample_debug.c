/*------------------------------------------------------------------------------------------------*/
/* Generation of a pseudo-random sample object.
 * Create an object with randomised contents that can be validated using a seed value */
/* Begun 2018-2019, StackHPC Ltd */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <sys/types.h>

#include "utils.h"
#include "sample.h"
#include "sample_priv.h"

/*------------------------------------------------------------------------------------------------*/
/* Simple implementation of data object samples for debug */

/* NOTE: sample_debug_len_max must be an integral number of uint32_t words.  We depend on this. */
static const size_t sample_debug_len_min = SAMPLE_LEN_MAX/2, sample_debug_len_max = SAMPLE_LEN_MAX;

struct sample_s
{
    size_t len;
    uint32_t *data;
};

static size_t sample_debug_len_calc( prng_t *P )
{
    return prng_next(P) % (sample_debug_len_max - sample_debug_len_min) + sample_debug_len_min;
}


/*------------------------------------------------------------------------------------------------*/

/* Initialise and finalise a pre-allocated sample object */
/* Initialise can be used to reset a sample to a seed value */
static sample_t *sample_debug_init( sample_t *S, prng_t *P )
{
    S->len = sample_debug_len_calc( P );
    assert( S->len <= sample_debug_len_max );       /* Paranoia */

    const unsigned whole_words = S->len / sizeof(uint32_t);
    const unsigned remain = S->len % sizeof(uint32_t);

    for( unsigned i=0; i < whole_words; i++ )
        S->data[i] = prng_next( P );

    /* Write a final word if there was a non-zero byte remainder */
    /* NOTE: we depend on sample_debug_len_max being a unit number of uint32_t words */
    if( remain ) S->data[whole_words] = prng_next( P );

    return S;
}

static void sample_debug_read( sample_t *S, const void *data, const size_t len )
{
    assert( len <= SAMPLE_LEN_MAX );
    S->len = len;
    memcpy( S->data, data, len );
}

/* Compare a sample value with the PRNG sequence that generated it. */
static bool sample_debug_valid( sample_t *S, prng_t *P )
{
    /* NOTE: the PRNG sequence must be applied in the same order as upon init */
    const size_t valid_len = sample_debug_len_calc( P );
    if( S->len != valid_len )
    {
        fprintf( stderr, "Length mismatch: Wanted %zd, got %zd\n", valid_len, S->len );
        return false;
    }

    const unsigned whole_words = S->len / sizeof(uint32_t);
    const unsigned remain = S->len % sizeof(uint32_t);

    for( unsigned i=0; i < whole_words; i++ )
    {
        const uint32_t check_data = prng_next( P );
        if( check_data != S->data[i] )
        {
            fprintf( stderr, "Data mismatch at word %u: Wanted %08x got %08x\n", i, check_data, S->data[i] );
            return false;
        }
    }

    /* Check the final word if there was a non-zero byte remainder */

    /* NOTE: we depend on sample_debug_len_max being a unit number of uint32_t words */
    if( remain )
    {
        /* We can't use the same method as in buffer init: overspill space will not be read in */
        /* Use memcmp instead */
        const uint32_t final_word = prng_next( P );
        const void *check_remain = (const void *)&final_word;

        if( memcmp(check_remain, (const void *)(S->data + whole_words), remain ) != 0 )
        {
            fprintf( stderr, "Data mismatch at remainder of sample\n" );
            return false;
        }
    }

    return true;
}

static void sample_debug_fini( sample_t *S )
{
    /* Retain the memory allocated on fini, in case it is reused by a subsequent call to init */
}

/* Create and initialise a single sample object. */
/* This is treated as a special case of allocating mutiple objects */
static sample_t *sample_debug_create( prng_t *P )
{
    /* Alloc our sample object and initialise with a pseudo-randomised length and data */
    sample_t *S = malloc( sizeof(struct sample_s) );
    if( S != NULL )
    {
        S->data = malloc( sample_debug_len_max );       /* Always alloc the max, for reuse */
        if( S->data != NULL )
        {
            sample_debug_init( S, P );
        }
        else
        {
            free( S );
            S = NULL;
        }
    }
    return S;
}

/* Create and initialise a single sample object. */
/* This is treated as a special case of allocating mutiple objects */
static void sample_debug_destroy( sample_t *S )
{
    if( S != NULL )                                     /* Defensive */
    {
        if( S->data != NULL )                           /* Defensive */
        {
            free( S->data );
        }
        free( S );
    }
}

static size_t sample_debug_len( sample_t *S )
{
    return S->len;
}

static const void *sample_debug_data( sample_t *S )
{
    return S->data;
}

/*------------------------------------------------------------------------------------------------*/
/* Sample methods for this implementation */

sample_driver_t sample_debug = 
{
    .sample_create = sample_debug_create,
    .sample_destroy = sample_debug_destroy,
    .sample_init = sample_debug_init,
    .sample_read = sample_debug_read,
    .sample_fini = sample_debug_fini,
    .sample_valid = sample_debug_valid,
    .sample_len = sample_debug_len,
    .sample_data = sample_debug_data,
};
