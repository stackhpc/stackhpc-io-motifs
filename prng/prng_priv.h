/*------------------------------------------------------------------------------------------------*/
/* Private implementation details for pseudo-random number generator */

#ifndef __PRNG_PRIV_H__                                         /* __PRNG_PRIV_H__ */
#define __PRNG_PRIV_H__                                         /* __PRNG_PRIV_H__ */

typedef struct
{
    /* Create and destroy */
    prng_t *(*prng_create)( const uint32_t seed );
    void (*prng_destroy)( prng_t *P );

    /* Initialise and finalise a pre-allocated PRNG object */
    /* Initialise can be used to reset a PRNG to a seed value */
    prng_t *(*prng_init)( prng_t *P, const uint32_t seed );
    void (*prng_fini)( prng_t *P );

    /* Get the next pseudo-random number in the sequence */
    uint32_t (*prng_next)( prng_t *P );
    uint32_t (*prng_peek)( prng_t *P );

} prng_driver_t;

/* PRNG Implementations */
extern prng_driver_t prng_debug;
extern prng_driver_t prng_xorshift;

#endif                                                          /* __PRNG_PRIV_H__ */
