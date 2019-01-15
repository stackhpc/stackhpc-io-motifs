/*------------------------------------------------------------------------------------------------*/
/* Synchronization routines to support parallel execution. */
/* Begun 2019, StackHPC Ltd */

#include <stdio.h>
#include <stdbool.h>
#include "barrier.h"

/* 
 * Initialize a barrier structure in shared memory to allow inter-process
 * synchronization.
 */
barrier_t *barrier_init( const char *handle, const int count )
{
    int fd = shm_open( handle, O_RDWR | O_CREAT, 0666 );
    int size_needed = sizeof( barrier_t ) + strlen( handle ) + 1;

    if ( (fd < 0) || ftruncate( fd, size_needed )) 
    {
        return (void*)( -1 );
    }

    barrier_t *bp = mmap( NULL, size_needed, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0 );

    if( bp != MAP_FAILED ) 
    {
        bp->b_num = count;
        bp->b_count = 0;
        strcpy( bp->b_handle, handle );
        sem_init( &bp->b_mutex, 1, 1 );
        sem_init( &bp->b_barrier, 1, 0 );
    }
    close( fd );
    return( bp );
}

/* Cleanup barrier resources */
void barrier_destroy( barrier_t *bp )
{
    char handle[strlen( bp->b_handle )];

    strcpy( handle, bp->b_handle );
    sem_destroy( &bp->b_mutex );
    sem_destroy( &bp->b_barrier );
    munmap( bp, sizeof( *bp )  + strlen( bp->b_handle ) + 1 );
    shm_unlink( handle );
}

void barrier_wait( barrier_t *bp )
{
    bool signal = 0;
    sem_wait( &bp->b_mutex );
    if( (signal = (++bp->b_count == bp->b_num)) )
    {
         bp->b_count = 0;   /* reset barrier for next use */
    }
    sem_post( &bp->b_mutex );

    if( signal )
    {
        sem_post( &bp->b_barrier );
    }

    sem_wait( &bp->b_barrier );	/* wait for barrier to free */
    sem_post( &bp->b_barrier ); /* notify next waiter */
}

