/*------------------------------------------------------------------------------------------------*/
/* Storage and retrieval of pseudo-random sample objects.
 * Write an object (with a pre-determined filename) to storage.
 * Read back an object for subsequent validation. */
/* Begun 2018-2019, StackHPC Ltd */

#include <sys/types.h>

#include "sample.h"

#ifndef __STORAGE_H__                                           /* __STORAGE_H__ */
#define __STORAGE_H__                                           /* __STORAGE_H__ */

/* Set up a storage driver on application startup */
/* For file-based storage implementations, the workspace is a directory pathname */
/* NOTE: must be called after the PRNG implementation has been selected */
extern int storage_create( const char *workspace );

/* Cleanup state from a storage driver on application shutdown */
extern int storage_destroy( void );

/* Write a sample object to storage */
extern int storage_write( const uint32_t client_id, const uint32_t obj_id, sample_t *S );

/* Read a sample object from storage */
extern int storage_read( const uint32_t client_id, const uint32_t obj_id, sample_t *S );

/* Select an implementation of storage backend.
 * NOTE: this cannot be done while the application is active */
typedef enum storage_impl
{
    STORAGE_DEBUG,             /* Default */
    STORAGE_DIRTREE,
} storage_impl_t;

#define STORAGE_IMPL_STR 	{ "DEBUG", "DIRTREE", NULL }

extern void storage_select( storage_impl_t impl );

#endif                                                          /* __STORAGE_H__ */
