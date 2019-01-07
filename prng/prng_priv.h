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

    /* Allocate and initialise an array of PRNG objects */
    /* They will be initialised with seeds generated in a PRNG sequence from the supplied seed */
    prng_t *(*prng_create_array)( const unsigned nobj, const uint32_t master_seed );
    void (*prng_destroy_array)( prng_t *P );

    /* Get the next pseudo-random number in the sequence */
    uint32_t (*prng_get)( prng_t *P );
    uint32_t (*prng_peek)( prng_t *P );

} prng_driver_t;

/* PRNG Implementations */
extern prng_driver_t prng_debug;

#endif                                                          /* __PRNG_PRIV_H__ */
