/*------------------------------------------------------------------------------------------------*/
/* Pseudo-random number generator:
 * Generate useful random number sequences that are repeatable based on a seed value.
 * Support multiple concurrent random number sequences. */
/* Begun 2018-2019, StackHPC Ltd */

#include <stdint.h>

#ifndef __PRNG_H__                                              /* __PRNG_H__ */
#define __PRNG_H__                                              /* __PRNG_H__ */

/* Opaque datatype for PRNG objects */
/* Each prng_t object will generate an independent sequence of random numbers */
typedef struct prng_s prng_t;

/* Create and destroy */
extern prng_t *prng_create( const uint32_t seed );
extern void prng_destroy( prng_t *P );

/* Initialise and finalise a pre-allocated PRNG object */
/* Initialise can be used to reset a PRNG to a seed value */
extern prng_t *prng_init( prng_t *P, const uint32_t seed );
extern void prng_fini( prng_t *P );

/* Get the next pseudo-random number in the sequence */
extern uint32_t prng_next( prng_t *P );

/* Retrieve the next pseudo-random number without advancing the sequence */
extern uint32_t prng_peek( prng_t *P );

/* Select an implementation of PRNG
 * NOTE: this cannot be done while PRNG objects are in use */
typedef enum prng_impl
{
    PRNG_DEBUG,             /* Default */
    PRNG_XORSHIFT,          /* xor shift */
} prng_impl_t;

#define PRNG_IMPL_STR 	{ "DEBUG", "XORSHIFT", NULL }

extern void prng_select( prng_impl_t impl );

#endif                                                          /* __PRNG_H__ */
