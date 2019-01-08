/*------------------------------------------------------------------------------------------------*/
/* Generation of a pseudo-random sample object.
 * Create an object with randomised contents that can be validated using a seed value */
/* Begun 2018-2019, StackHPC Ltd */

#include "utils.h"
#include "sample.h"
#include "sample_priv.h"

/*------------------------------------------------------------------------------------------------*/

/* Pointer to sample implementation selected */
static sample_driver_t *sample = &sample_debug;

/* sample implementation selector */
void sample_select( sample_impl_t impl )
{
    static const struct { sample_impl_t impl; sample_driver_t *driver; } sample_drivers[] = 
    {
        { SAMPLE_DEBUG, &sample_debug },
    };

    for( unsigned i=0; i < ARRAYLEN(sample_drivers); i++ )
    {
        if( sample_drivers[i].impl == impl )
        {
            sample = sample_drivers[i].driver;
            break;
        }
    }
}

/*------------------------------------------------------------------------------------------------*/
/* Create and initialise a single sample object. */
/* This is treated as a special case of allocating mutiple objects */
sample_t *sample_create( prng_t *P )
{
    return sample->sample_create( P );
}

/* Create and initialise a single sample object. */
/* This is treated as a special case of allocating mutiple objects */
void sample_destroy( sample_t *P )
{
    sample->sample_destroy( P );
}


/* Initialise and finalise a pre-allocated sample object */
/* Initialise can be used to reset a sample to a seed value */
sample_t *sample_init( sample_t *S, prng_t *P )
{
    return sample->sample_init( S, P );
}

void sample_read( sample_t *S, const void *data, const size_t len )
{
    sample->sample_read( S, data, len );
}

void sample_fini( sample_t *S )
{
    sample->sample_fini( S );
}

bool sample_valid( sample_t *S, prng_t *P )
{
    return sample->sample_valid( S, P );
}

size_t sample_len( sample_t *S )
{
    return sample->sample_len( S );
}

void const *sample_data( sample_t *S )
{
    return sample->sample_data( S );
}
