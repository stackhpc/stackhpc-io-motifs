/*------------------------------------------------------------------------------------------------*/
/* Private implementation details for storage and retrieval implementations */

#include "sample.h"

#ifndef __STORAGE_PRIV_H__                                       /* __STORAGE_PRIV_H__ */
#define __STORAGE_PRIV_H__                                       /* __STORAGE_PRIV_H__ */

typedef struct
{
    /* Set up a storage driver on application startup */
    /* For file-based storage implementations, the workspace is a directory pathname */
    /* NOTE: must be called after the PRNG implementation has been selected */
    int (*storage_create)( const char *workspace );

    /* Cleanup state from a storage driver on application shutdown */
    int (*storage_destroy)( void );

    /* Write a sample object to storage */
    int (*storage_write)( const uint32_t client_id, const uint32_t obj_id, sample_t *S );

    /* Read a sample object from storage */
    int (*storage_read)( const uint32_t client_id, const uint32_t obj_id, sample_t *S );

} storage_driver_t;

/* Storage driver implementations */
extern storage_driver_t storage_debug;
extern storage_driver_t storage_dirtree;

#endif                                                          /* __STORAGE_PRIV_H__ */
