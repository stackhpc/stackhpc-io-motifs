/*------------------------------------------------------------------------------------------------*/
/* Private implementation details for randomised data sample objects */

#include "prng.h"

#ifndef __SAMPLE_PRIV_H__                                       /* __SAMPLE_PRIV_H__ */
#define __SAMPLE_PRIV_H__                                       /* __SAMPLE_PRIV_H__ */

typedef struct
{
    /* Create and destroy */
    sample_t *(*sample_create)( prng_t *P );
    void (*sample_destroy)( sample_t *S );

    /* Initialise and finalise a pre-allocated SAMPLE object */
    /* Initialise can be used to reset a SAMPLE to a seed value */
    sample_t *(*sample_init)( sample_t *S, prng_t *P );
    void (*sample_fini)( sample_t *S );

    bool (*sample_valid)( sample_t *S, prng_t *P );

    size_t (*sample_len)( sample_t *S );

    const void *(*sample_data)( sample_t *S );

} sample_driver_t;

/* Sample implementations */
extern sample_driver_t sample_debug;

#endif                                                          /* __SAMPLE_PRIV_H__ */
