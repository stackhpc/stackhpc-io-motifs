/*------------------------------------------------------------------------------------------------*/
/* Timestamping and time manipulation utility routines. 
 * Get current time with nanosecond resolution and manipulate timestamps. */
/* Begun 2019, StackHPC Ltd */

#include <time.h>

#include "utils.h"

/* Get current time in seconds and nanoseconds */
void time_now( struct timespec *ts )
{
    /* CLOCK_MONOTONIC is better because it cannot be changed (and therefore never goes backwards).
     * However, we also expect to be comparing data across multiple hosts.
     * Solution: assume a start time synchronised across all hosts is sufficient
     * and use timestamps relative to this event on all hosts. */
    clock_gettime( CLOCK_MONOTONIC, ts );
}

/* Calculate the time difference between t1 and t2, in seconds and nanoseconds (NB: t1 < t2) */
void time_delta( const struct timespec *t1, const struct timespec *t2, struct timespec *delta )
{
    delta->tv_sec = t2->tv_sec - t1->tv_sec;
    delta->tv_nsec = t2->tv_nsec - t1->tv_nsec;     /* This may wrap (and go negative) */

    /* Carry over a second, if the nanoseconds are negative.
     * Division is also possible but we expect the delta to be sub-second. */
    while( delta->tv_nsec < 0L )
    {
        delta->tv_sec--;
        delta->tv_nsec += 1000000000L;
    }
}

