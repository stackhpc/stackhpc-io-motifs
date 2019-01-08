/*------------------------------------------------------------------------------------------------*/
/* Generation of a pseudo-random sample object.
 * Create an object with randomised contents that can be validated using a seed value */
/* Begun 2018-2019, StackHPC Ltd */

#include <stdbool.h>

#include <sys/types.h>

#include "prng.h"

#ifndef __SAMPLE_H__                                              /* __SAMPLE_H__ */
#define __SAMPLE_H__                                              /* __SAMPLE_H__ */

/* NOTE: byte length defined here must be an integral number of 32-bit words */
#define SAMPLE_LEN_WORDS    384
#define SAMPLE_LEN_MAX      (SAMPLE_LEN_WORDS * sizeof(uint32_t))

/* Opaque datatype for sample objects */
typedef struct sample_s sample_t;

/* Generate a sample data object */
extern sample_t *sample_create( prng_t *P );

/* Deallocate a sample data object */
extern void sample_destroy( sample_t *S );

/* Re-initialise a pre-allocated sample data object */
extern sample_t *sample_init( sample_t *S, prng_t *P );

/* Initialise a sample_t object (NB, unvalidated) from storage data */
extern void sample_read( sample_t *S, const void *data, const size_t len );

/* Finalise a sample data object (de-initialise without deallocation) */
extern void sample_fini( sample_t *S );

/* Validate a sample data object that has been read back */
extern bool sample_valid( sample_t *S, prng_t *P );

/* Retrieve the length of a sample data object */
extern size_t sample_len( sample_t *S );

/* Access the data from a sample data object */
extern const void *sample_data( sample_t *S );

/* Select an implementation of sample data object
 * NOTE: this cannot be done while sample objects are in use */
typedef enum sample_impl
{
    SAMPLE_DEBUG,             /* Default */
} sample_impl_t;
extern void sample_select( sample_impl_t impl );

#endif                                                          /* __SAMPLE_H__ */
