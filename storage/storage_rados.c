/*------------------------------------------------------------------------------------------------*/
/* Storage and retrieval of pseudo-random sample objects.
 * Write an object (with a pre-determined filename) to storage. 
 * Read back an object for subsequent validation. */
/* Begun 2019, StackHPC Ltd */

#include <stdint.h>
#include <string.h>
#include <limits.h>

#include <rados/librados.h>

#include "utils.h"
#include "sample.h"
#include "storage.h"
#include "storage_priv.h"

/*------------------------------------------------------------------------------------------------*/
/* Simple implementation of file-based storage */

static rados_t storage_rados_data;
static rados_ioctx_t storage_rados_ctx;
static char *storage_rados_pool = "benchmark";
static char *storage_rados_ceph_conf = "ceph.conf";
/*static char *storage_rados_ceph_conf = "/etc/ceph/ceph.conf";*/

/* Set up a storage driver on application startup */
/* For file-based storage implementations, the workspace is a directory pathname */
static int storage_rados_create( const char *workspace, int argc, char *argv[] )
{

    const int rados_err = rados_create( &storage_rados_data, NULL );
    if( rados_err < 0 )
    {
        log_error( "Error creating RADOS object: %s", strerror(-rados_err) );
        return rados_err;
    }

    const int conf_err = rados_conf_read_file( storage_rados_data, storage_rados_ceph_conf );
    if( conf_err < 0 )
    {
        log_error( "Error reading Ceph configuration: %s", strerror(-conf_err) );
        return conf_err;
    }

    const int argv_err = rados_conf_parse_argv( storage_rados_data, argc, (const char **)argv );
    if( argv_err < 0 )
    {
        log_error( "Error parsing args for Ceph configuration: %s", strerror(-argv_err) );
        return argv_err;
    }

    const int connect_err = rados_connect( storage_rados_data );
    if( connect_err < 0 )
    {
        log_error( "Error connecting to Ceph cluster: %s", strerror(-connect_err) );
        return connect_err;
    }

    const int ioctx_err = rados_ioctx_create( storage_rados_data, storage_rados_pool,
                                             &storage_rados_ctx );
    if( ioctx_err < 0 )
    {
        log_error( "Error creating Ceph I/O context for pool '%s': %s",
                    storage_rados_pool, strerror(-ioctx_err) );
        rados_shutdown( storage_rados_data );
        return ioctx_err;
    }

    log_info( "Connected to Ceph cluster, pool %s", storage_rados_pool );
    return 0;
}

/* Cleanup state from a storage driver on application shutdown */
static int storage_rados_destroy( void )
{
    rados_ioctx_destroy( storage_rados_ctx );
    rados_shutdown( storage_rados_data ); 
    return 0;
}

/* Write a sample object to storage */
static int storage_rados_write( const uint32_t client_id, const uint32_t obj_id, sample_t *S )
{
    struct timespec iop_start, iop_end, iop_delta, ts_delta;
    char filename[20];
    snprintf( filename, sizeof(filename), "%08x-%08x", client_id, obj_id );

    time_now( &iop_start );

    const int rados_err = rados_write_full( storage_rados_ctx, filename,
                                            sample_data(S), sample_len(S) );
    if( rados_err < 0 )
    {
        log_error( "Cannot write %zd-byte object %s to pool %s: %s\n",
                    sample_len(S), filename, storage_rados_pool, strerror(-rados_err) );
        return rados_err;
    }

    time_now( &iop_end );
    time_delta( &iop_start, &iop_end, &iop_delta );
    time_delta( &time_benchmark, &iop_start, &ts_delta );
    trace_write( &ts_delta, &iop_delta );

    return 0;
}

/* Read a sample object from storage */
static int storage_rados_read( const uint32_t client_id, const uint32_t obj_id, sample_t *S )
{
    struct timespec iop_start, iop_end, iop_delta, ts_delta;
    char obj_data[SAMPLE_LEN_MAX];
    char filename[20];
    snprintf( filename, sizeof(filename), "%08x-%08x", client_id, obj_id );

    time_now( &iop_start );

    const int rados_result = rados_read( storage_rados_ctx, filename,
                                         obj_data, sizeof(obj_data), 0UL );
    if( rados_result < 0 )
    {
        log_error( "Cannot read object %s from pool %s: %s\n",
                    filename, storage_rados_pool, strerror(-rados_result) );
        return rados_result;
    }

    time_now( &iop_end );
    time_delta( &iop_start, &iop_end, &iop_delta );
    time_delta( &time_benchmark, &iop_start, &ts_delta );
    trace_read( &ts_delta, &iop_delta );

    /* Transfer the data into our sample object */
    sample_read( S, obj_data, rados_result );
    return 0;
}


/*------------------------------------------------------------------------------------------------*/
/* Storage methods for this implementation */

storage_driver_t storage_rados = 
{
    .storage_create = storage_rados_create,
    .storage_destroy = storage_rados_destroy,
    .storage_write = storage_rados_write,
    .storage_read = storage_rados_read,
};
