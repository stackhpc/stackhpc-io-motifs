/*------------------------------------------------------------------------------------------------*/
/* Storage and retrieval of pseudo-random sample objects.
 * Write an object (with a pre-determined filename) to storage. 
 * Read back an object for subsequent validation. */
/* Begun 2018-2019, StackHPC Ltd */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <dirent.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "utils.h"
#include "prng.h"
#include "sample.h"
#include "storage.h"
#include "storage_priv.h"

/*------------------------------------------------------------------------------------------------*/
/* Simple implementation of file-based storage */

static char *storage_debug_workspace = NULL;
static char storage_debug_cwd[PATH_MAX];

/* Set up a storage driver on application startup */
/* For file-based storage implementations, the workspace is a directory pathname */
static int storage_debug_create( const char *workspace )
{
    struct stat st;
    const int st_result = stat( workspace, &st );
    if( st_result < 0 )
    {
        if( errno != ENOENT )
        {
            log_error( "Couldn't stat %s: %s", workspace, strerror(errno) );
            return -1;
        }
    }
    else
    {
        log_error( "Workspace %s already exists: cannot proceed", workspace );
        return -1;
    }

    /* Prepare the workspace */
    assert( storage_debug_workspace == NULL );
    storage_debug_workspace = strndup( workspace, PATH_MAX );
    if( storage_debug_workspace == NULL )
    {
        log_error( "Insufficient memory to alloc state for workspace %s", workspace );
        return -1;
    }
    const int mkdir_result = mkdir( workspace, 0755 );
    if( mkdir_result < 0 )
    {
        log_error( "Workspace %s could not be created: %s", workspace, strerror(errno) );
        free( storage_debug_workspace );
        storage_debug_workspace = NULL;
        return -1;
    }

    /* Change directory into the workspace */
    getcwd( storage_debug_cwd, sizeof(storage_debug_cwd) );
    log_trace( "Entering workspace %s", storage_debug_workspace );
    const int chdir_result = chdir( storage_debug_workspace );
    if( chdir_result < 0 )
    {
        log_error( "Workspace %s could not be entered: %s", workspace, strerror(errno) );
        rmdir( storage_debug_workspace );
        free( storage_debug_workspace );
        storage_debug_workspace = NULL;
        return -1;
    }

    /* Allocate a PRNG object for use in generating files */
    return 0;
}

/* Cleanup state from a storage driver on application shutdown */
static int storage_debug_destroy( void )
{
    /* Deallocate the workspace (this might take a while...) */
    if( storage_debug_workspace != NULL )
    {
        /* Erase all files in the directory */
        DIR *D = opendir( "." );
        if( D != NULL )
        {
            struct dirent *obj;
            while( (obj=readdir(D)) != NULL )
            {
                struct stat st;
                const int stat_result = stat( obj->d_name, &st );
                if( stat_result < 0 )
                {
                    log_error( "Couldn't stat object %s prior to removal", obj->d_name );
                    continue;
                }

                if( S_ISREG(st.st_mode) )
                {
                    log_trace( "Removing object %s", obj->d_name );
                    unlink( obj->d_name );
                }
            }
            closedir( D );
        }

        log_trace( "Returning to %s", storage_debug_cwd );
        chdir( storage_debug_cwd );
        /* FIXME: we need to empty the directory first - this won't work */
        const int rmdir_result = rmdir( storage_debug_workspace );
        if( rmdir_result < 0 )
        {
            log_error( "Unable to remove workspace dir %s: %s", storage_debug_workspace, strerror(errno) );
        }
        free( storage_debug_workspace );
        storage_debug_workspace = NULL;
    }
    return 0;
}

/* Write a sample object to storage */
static int storage_debug_write( const uint32_t client_id, const uint32_t obj_id, sample_t *S )
{
    struct timespec iop_start, iop_end, iop_delta, ts_delta;
    char filename[20];
    snprintf( filename, sizeof(filename), "%08x-%08x", client_id, obj_id );

    time_now( &iop_start );
    const int fd = open( filename, O_CREAT|O_EXCL|O_WRONLY, 0644 );
    if( fd < 0 )
    {
        log_error( "Unable to create+open file %s: %s", filename, strerror(errno) );
        return -1;
    }

    const ssize_t write_result = write( fd, sample_data(S), sample_len(S) );
    if( write_result != sample_len(S) )
    {
        log_error( "Error %zd writing data to fd %d file %s: %s", write_result, fd, filename, strerror(errno) );
        return -1;
    }

    const int close_result = close( fd );
    if( close_result < 0 )
    {
        log_error( "Unable to close file %s: %s", filename, strerror(errno) );
        return -1;
    }
    time_now( &iop_end );
    time_delta( &iop_start, &iop_end, &iop_delta );
    time_delta( &time_benchmark, &iop_start, &ts_delta );
    trace_write( &ts_delta, &iop_delta );

    return 0;
}

/* Read a sample object from storage */
static int storage_debug_read( const uint32_t client_id, const uint32_t obj_id, sample_t *S )
{
    struct timespec iop_start, iop_end, iop_delta, ts_delta;
    uint8_t obj_data[SAMPLE_LEN_MAX];
    char filename[20];
    snprintf( filename, sizeof(filename), "%08x-%08x", client_id, obj_id );

    time_now( &iop_start );
    const int fd = open( filename, O_RDONLY );
    if( fd < 0 )
    {
        log_error( "Unable to open file %s: %s", filename, strerror(errno) );
        return -1;
    }

    struct stat st;
    const int st_result = fstat( fd, &st );
    if( st_result < 0 )
    {
        log_error( "Unable to stat file %s: %s", filename, strerror(errno) );
        return -1;
    }
    assert( st.st_size <= SAMPLE_LEN_MAX );

    /* We'd like to cut out a copy here by loading data direct into the sample data object */
    const ssize_t read_result = read( fd, obj_data, st.st_size );
    if( read_result != st.st_size )
    {
        log_error( "Error %zd loading data from file %s: %s", read_result, filename, strerror(errno) );
        return -1;
    }

    const int close_result = close( fd );
    if( close_result < 0 )
    {
        log_error( "Unable to close file %s: %s", filename, strerror(errno) );
        return -1;
    }
    time_now( &iop_end );
    time_delta( &iop_start, &iop_end, &iop_delta );
    time_delta( &time_benchmark, &iop_start, &ts_delta );
    trace_read( &ts_delta, &iop_delta );

    /* Transfer the data into our sample object */
    sample_read( S, obj_data, st.st_size );
    return 0;
}


/*------------------------------------------------------------------------------------------------*/
/* Storage methods for this implementation */

storage_driver_t storage_debug = 
{
    .storage_create = storage_debug_create,
    .storage_destroy = storage_debug_destroy,
    .storage_write = storage_debug_write,
    .storage_read = storage_debug_read,
};
