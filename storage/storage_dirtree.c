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
/* Using multiple levels of directory hierarchy to prevent directory catalogues from growing 
 * to become unmanageable. */

static char *storage_dirtree_workspace = NULL;
static char storage_dirtree_cwd[PATH_MAX];

static char *storage_dirtree_pathname( char *buf, const uint32_t client_id, const uint32_t obj_id )
{
    sprintf( buf, "%04X/%04X/%04X/%08X-%08X",
	     client_id & 0xFFFFU, (client_id >> 16) & 0xFFFFU,
             (obj_id >> 16) & 0xFFFFU, client_id, obj_id );
    return buf;
}

static int storage_dirtree_pathgen( const uint32_t client_id, const uint32_t obj_id )
{
    char buf[24];

    sprintf( buf, "%04X", client_id & 0xFFFFU );
    mkdir( buf, 0755 );

    sprintf( buf, "%04X/%04X",
	     client_id & 0xFFFFU, (client_id >> 16) & 0xFFFFU );
    mkdir( buf, 0755 );

    sprintf( buf, "%04X/%04X/%04X",
	     client_id & 0xFFFFU, (client_id >> 16) & 0xFFFFU,
             (obj_id >> 16) & 0xFFFFU );
    const int mkdir_result = mkdir( buf, 0755 );
    return mkdir_result;
}


/* Set up a storage driver on application startup */
/* For file-based storage implementations, the workspace is a directory pathname */
static int storage_dirtree_create( const char *workspace, int argc, const char *argv[] )
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
    assert( storage_dirtree_workspace == NULL );
    storage_dirtree_workspace = strndup( workspace, PATH_MAX );
    if( storage_dirtree_workspace == NULL )
    {
        log_error( "Insufficient memory to alloc state for workspace %s", workspace );
        return -1;
    }
    const int mkdir_result = mkdir( workspace, 0755 );
    if( mkdir_result < 0 )
    {
        log_error( "Workspace %s could not be created: %s", workspace, strerror(errno) );
        free( storage_dirtree_workspace );
        storage_dirtree_workspace = NULL;
        return -1;
    }

    /* Change directory into the workspace */
    getcwd( storage_dirtree_cwd, sizeof(storage_dirtree_cwd) );
    log_trace( "Entering workspace %s", storage_dirtree_workspace );
    const int chdir_result = chdir( storage_dirtree_workspace );
    if( chdir_result < 0 )
    {
        log_error( "Workspace %s could not be entered: %s", workspace, strerror(errno) );
        rmdir( storage_dirtree_workspace );
        free( storage_dirtree_workspace );
        storage_dirtree_workspace = NULL;
        return -1;
    }

    /* Allocate a PRNG object for use in generating files */
    return 0;
}


static void storage_dirtree_rmdir( void )
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

            if( strcmp(obj->d_name, ".")==0 || strcmp(obj->d_name, "..")==0 )
            {
                continue;
            }

            if( S_ISREG(st.st_mode) )
            {
                //log_trace( "Removing object %s", obj->d_name );
                unlink( obj->d_name );
            }
            if( S_ISDIR(st.st_mode) )
            {
                char cwd[PATH_MAX];

                log_debug( "Pruning directory %s", obj->d_name );

                getcwd( cwd, sizeof(cwd) );
                chdir( obj->d_name );
                storage_dirtree_rmdir( );
                chdir( cwd );
                rmdir( obj->d_name );
            }
        }
        closedir( D );
    }
}

/* Cleanup state from a storage driver on application shutdown */
static int storage_dirtree_destroy( void )
{
    /* Deallocate the workspace (this might take a while...) */
    if( storage_dirtree_workspace != NULL )
    {
        /* Recursively remove files and directories in the workspace */
        storage_dirtree_rmdir( );

        log_trace( "Returning to %s", storage_dirtree_cwd );
        chdir( storage_dirtree_cwd );
    
        const int rmdir_result = rmdir( storage_dirtree_workspace );
        if( rmdir_result < 0 )
        {
            log_error( "Unable to remove workspace dir %s: %s", storage_dirtree_workspace, strerror(errno) );
        }
        free( storage_dirtree_workspace );
        storage_dirtree_workspace = NULL;
    }
    return 0;
}

/* Write a sample object to storage */
static int storage_dirtree_write( const uint32_t client_id, const uint32_t obj_id, sample_t *S )
{
    struct timespec iop_start, iop_end, iop_delta, ts_delta;
    char filename[48];
    storage_dirtree_pathname( filename, client_id, obj_id );

    time_now( &iop_start );
    int fd = open( filename, O_CREAT|O_EXCL|O_WRONLY, 0644 );
    if( fd < 0 )
    {
        /* Generate the directory path and try again */
        storage_dirtree_pathgen( client_id, obj_id );
        time_now( &iop_start );
        fd = open( filename, O_CREAT|O_EXCL|O_WRONLY, 0644 );
        if( fd < 0 )
        {
            log_error( "Unable to create+open file %s: %s", filename, strerror(errno) );
            return -1;
        }
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
static int storage_dirtree_read( const uint32_t client_id, const uint32_t obj_id, sample_t *S )
{
    struct timespec iop_start, iop_end, iop_delta, ts_delta;
    uint8_t obj_data[SAMPLE_LEN_MAX];
    char filename[48];
    storage_dirtree_pathname( filename, client_id, obj_id );

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

storage_driver_t storage_dirtree = 
{
    .storage_create = storage_dirtree_create,
    .storage_destroy = storage_dirtree_destroy,
    .storage_write = storage_dirtree_write,
    .storage_read = storage_dirtree_read,
};
