/*------------------------------------------------------------------------------------------------*/
/* Pseudo-random number generator:
 * Generate useful random number sequences that are repeatable based on a seed value.
 * Support multiple concurrent random number sequences. */
/* Begun 2018-2019, StackHPC Ltd */

#include <stdint.h>
#include <stdlib.h>

#include "utils.h"
#include "prng.h"
#include "prng_priv.h"

/*------------------------------------------------------------------------------------------------*/

/* Pointer to PRNG implementation selected */
static prng_driver_t *PRNG = &prng_debug;

/* PRNG implementation selector */
void prng_select( prng_impl_t impl )
{
    static const struct { prng_impl_t impl; prng_driver_t *driver; } prng_drivers[] = 
    {
        { PRNG_DEBUG, &prng_debug },
    };

    for( unsigned i=0; i < ARRAYLEN(prng_drivers); i++ )
    {
        if( prng_drivers[i].impl == impl )
        {
            PRNG = prng_drivers[i].driver;
            break;
        }
    }
}

/* Create and initialise a single PRNG object. */
/* This is treated as a special case of allocating mutiple objects */
prng_t *prng_create( const uint32_t seed )
{
    return PRNG->prng_create( seed );
}

/* Create and initialise a single PRNG object. */
/* This is treated as a special case of allocating mutiple objects */
void prng_destroy( prng_t *P )
{
    PRNG->prng_destroy( P );
}

/* Get the next pseudo-random number in the sequence */
uint32_t prng_random( prng_t *P )
{
    return PRNG->prng_random( P );
}
