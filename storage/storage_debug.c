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
static prng_t *PRNG = NULL;

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
            fprintf( stderr, "Couldn't stat %s: %s\n", workspace, strerror(errno) );
            return -1;
        }
    }
    else
    {
        fprintf( stderr, "Workspace %s already exists: cannot proceed\n", workspace );
        return -1;
    }

    /* Prepare the workspace */
    assert( storage_debug_workspace == NULL );
    storage_debug_workspace = strndup( workspace, PATH_MAX );
    if( storage_debug_workspace == NULL )
    {
        fprintf( stderr, "Insufficient memory to alloc state for workspace %s\n", workspace );
        return -1;
    }
    const int mkdir_result = mkdir( workspace, 0755 );
    if( mkdir_result < 0 )
    {
        fprintf( stderr, "Workspace %s could not be created: %s\n", workspace, strerror(errno) );
        free( storage_debug_workspace );
        storage_debug_workspace = NULL;
        return -1;
    }

    /* Change directory into the workspace */
    getcwd( storage_debug_cwd, sizeof(storage_debug_cwd) );
    printf( "Entering workspace %s\n", storage_debug_workspace );
    const int chdir_result = chdir( storage_debug_workspace );
    if( chdir_result < 0 )
    {
        fprintf( stderr, "Workspace %s could not be entered: %s\n", workspace, strerror(errno) );
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
        printf( "Returning to %s\n", storage_debug_cwd );
        chdir( storage_debug_cwd );
        /* FIXME: we need to empty the directory first - this won't work */
        const int rmdir_result = rmdir( storage_debug_workspace );
        if( rmdir_result < 0 )
        {
            fprintf(stderr, "Unable to remove workspace dir %s: %s\n", storage_debug_workspace, strerror(errno) );
        }
        free( storage_debug_workspace );
        storage_debug_workspace = NULL;
    }
    return 0;
}

/* Write a sample object to storage */
static int storage_debug_write( const uint32_t client_id, const uint32_t obj_id, sample_t *S )
{
    char filename[20];
    snprintf( filename, sizeof(filename), "%08x-%08x", client_id, obj_id );
    const int fd = open( filename, O_CREAT|O_EXCL|O_WRONLY, 0644 );
    if( fd < 0 )
    {
        fprintf( stderr, "Unable to create+open file %s: %s\n", filename, strerror(errno) );
        return -1;
    }

    const ssize_t write_result = write( fd, sample_data(S), sample_len(S) );
    if( write_result != sample_len(S) )
    {
        fprintf( stderr, "Error %zd writing data to fd %d file %s: %s\n", write_result, fd, filename, strerror(errno) );
        return -1;
    }

    const int close_result = close( fd );
    if( close_result < 0 )
    {
        fprintf( stderr, "Unable to close file %s: %s\n", filename, strerror(errno) );
        return -1;
    }

    return 0;
}

/* Read a sample object from storage */
static int storage_debug_read( const uint32_t client_id, const uint32_t obj_id, sample_t *S )
{
    uint8_t obj_data[SAMPLE_LEN_MAX];
    char filename[20];
    snprintf( filename, sizeof(filename), "%08x-%08x", client_id, obj_id );

    const int fd = open( filename, O_RDONLY );
    if( fd < 0 )
    {
        fprintf( stderr, "Unable to open file %s: %s\n", filename, strerror(errno) );
        return -1;
    }

    struct stat st;
    const int st_result = fstat( fd, &st );
    if( st_result < 0 )
    {
        fprintf( stderr, "Unable to stat file %s: %s\n", filename, strerror(errno) );
        return -1;
    }
    assert( st.st_size <= SAMPLE_LEN_MAX );

    /* We'd like to cut out a copy here by loading data direct into the sample data object */
    const ssize_t read_result = read( fd, obj_data, st.st_size );
    if( read_result != st.st_size )
    {
        fprintf( stderr, "Error %zd loading data from file %s: %s\n", read_result, filename, strerror(errno) );
        return -1;
    }

    const int close_result = close( fd );
    if( close_result < 0 )
    {
        fprintf( stderr, "Unable to close file %s: %s\n", filename, strerror(errno) );
        return -1;
    }

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
