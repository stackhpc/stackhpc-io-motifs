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

/* A pthread is created by this task for periodic flush of buffered trace data */
extern int trace_init( const char *trace_dir, const uint32_t trace_id );

/* Complete tracing, flush buffers and close files, terminate the captive thread */
extern int trace_fini( void );

/* Record timestamp and elapsed delta for an IOP */
extern int trace_write( const struct timespec *ts, const struct timespec *iop );
extern int trace_read( const struct timespec *ts, const struct timespec *iop );

#endif                                                          /* __UTILS_H__ */
