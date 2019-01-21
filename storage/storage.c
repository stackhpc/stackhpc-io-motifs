/*------------------------------------------------------------------------------------------------*/
/* Storage and retrieval of pseudo-random sample objects.
 * Write an object (with a pre-determined filename) to storage.
 * Read back an object for subsequent validation. */
/* Begun 2018-2019, StackHPC Ltd */

#include "utils.h"
#include "storage.h"
#include "storage_priv.h"

/*------------------------------------------------------------------------------------------------*/

/* Pointer to storage implementation selected */
static storage_driver_t *storage = &storage_debug;

/* storage implementation selector */
void storage_select( storage_impl_t impl )
{
    static const struct { storage_impl_t impl; storage_driver_t *driver; } storage_drivers[] = 
    {
        { STORAGE_DEBUG, &storage_debug },
        { STORAGE_DIRTREE, &storage_dirtree },
        { STORAGE_RADOS, &storage_rados },
    };

    for( unsigned i=0; i < ARRAYLEN(storage_drivers); i++ )
    {
        if( storage_drivers[i].impl == impl )
        {
            storage = storage_drivers[i].driver;
            break;
        }
    }
}


/*------------------------------------------------------------------------------------------------*/
/* Set up a storage driver on application startup */
/* For file-based storage implementations, the workspace is a directory pathname */
/* NOTE: must be called after the PRNG implementation has been selected */
int storage_create( const char *workspace, int argc, const char *argv[] )
{
    return storage->storage_create( workspace, argc, argv );
}

/* Cleanup state from a storage driver on application shutdown */
int storage_destroy( void )
{
    return storage->storage_destroy( );
}

/* Write a sample object to storage */
int storage_write( const uint32_t client_id, const uint32_t obj_id, sample_t *S )
{
    return storage->storage_write( client_id, obj_id, S );
}

/* Read a sample object from storage */
int storage_read( const uint32_t client_id, const uint32_t obj_id, sample_t *S )
{
    return storage->storage_read( client_id, obj_id, S );
}
