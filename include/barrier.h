/*------------------------------------------------------------------------------------------------*/
/* Synchronization functions */
/* Begun 2019, StackHPC Ltd */

#ifndef __BARRIER_H__                                              /* __BARRIER_H__ */
#define __BARRIER_H__                                              /* __BARRIER_H__ */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>

typedef struct barrier {
    sem_t 		b_mutex;	/* count mutex */
    sem_t		b_barrier;	/* barrier */
    int 		b_count;	/* initialization value */
    int			b_num;		/* total number of participants */
    char		b_handle[];	/* handle to backing object */
} barrier_t;

barrier_t *barrier_init( const char *andle, const int count );
void barrier_destroy( barrier_t *bp );
void barrier_wait( barrier_t *bp );

#endif                                                          /* __BARRIER_H__ */
