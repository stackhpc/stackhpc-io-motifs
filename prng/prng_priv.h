/*------------------------------------------------------------------------------------------------*/
/* Private implementation details for pseudo-random number generator */

#ifndef __PRNG_PRIV_H__                                         /* __PRNG_PRIV_H__ */
#define __PRNG_PRIV_H__                                         /* __PRNG_PRIV_H__ */

typedef struct
{
    /* Create and destroy */
    prng_t *(*prng_create)( const uint32_t seed );
    void (*prng_destroy)( prng_t *P );

    /* Get the next pseudo-random number in the sequence */
    uint32_t (*prng_random)( prng_t *P );

} prng_driver_t;

/* PRNG Implementations */
extern prng_driver_t prng_debug;

#endif                                                          /* __PRNG_PRIV_H__ */
