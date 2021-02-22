/*------------------------------------------------------------------------------------------------*/
/* Utility support functions */
/* Begun 2018-2019, StackHPC Ltd */

#ifndef __UTILS_H__                                              /* __UTILS_H__ */
#define __UTILS_H__                                              /* __UTILS_H__ */

#include <stdint.h>
#include <time.h>

/* Return the number of members in a statically-defined array */
#define ARRAYLEN(x)     (sizeof(x) / sizeof(*x))

/*------------------------------------------------------------------------------------------------*/
/* Logging functions: Copyright (c) 2017 rxi (Adapted and extended by StackHPC, 2019) */

#include "log.h"

/*------------------------------------------------------------------------------------------------*/
/* Timestamping functions:
 * Get current time with nanosecond resolution and manipulate timestamps. */

/* Get current time in seconds and nanoseconds */
extern void time_now( struct timespec *ts );

/* Calculate the time difference between t1 and t2, in seconds and nanoseconds (NB: t1 < t2) */
extern void time_delta( const struct timespec *t1, const struct timespec *t2, struct timespec *delta );

/* Timestamps for relative time offsets */
extern struct timespec time_start;          /* A timestamp set at application startup. */
extern struct timespec time_benchmark;      /* A timestamp set at the start of the benchmark run. */

/*------------------------------------------------------------------------------------------------*/
/* Emitting performance traces.
 * Generating telemetry streams during benchmark execution */

/* Trace data needs to hold the following components:
 * - Timestamp of operation commencement, relative to benchmark start.
 * - Type of record:
 *   - Write (single timestamp)
 *   - Read (single timestamp)
 *
 * NOTE: the actual size of the I/O is assumed to be insignificant.
 */

typedef enum trace_type {
   TRACE_READ = 0,
   TRACE_WRITE,
   TRACE_MISC
} trace_type_t;

typedef struct trace_entry {
    struct {
       uint8_t op;		/* operation type */
       char tag[7];		/* arbitrary identifying tag */
    } info;
    struct timespec timestamp;  /* timestamp for entry */
    struct timespec duration;	/* duration of operation */
} trace_entry_t;

extern const char *trace_type_str( const trace_type_t T );

/* A pthread is created by this task for periodic flush of buffered trace data */
extern int trace_init( const char *trace_dir, const uint32_t trace_id );

/* Complete tracing, flush buffers and close files, terminate the captive thread */
extern int trace_fini( void );

extern int trace( const trace_type_t tt, const struct timespec *ts, 
                  const struct timespec *iop, const char *tag );

/* Record timestamp and elapsed delta for an IOP */
static inline int
trace_read( const struct timespec *ts, const struct timespec *iop )
{
    return trace( TRACE_READ, ts, iop, NULL );
}
static inline int
trace_write( const struct timespec *ts, const struct timespec *iop )
{
    return trace( TRACE_WRITE, ts, iop, NULL );
}

#endif                                                          /* __UTILS_H__ */
