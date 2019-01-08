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

/* Implementation:
 * - binary log records are assumed
 * - 8-byte alignment should be maintained
 * - It should be fine to emit a logfile per type of IOP, with IOP type implicit.
 */

/* A pthread is created by this task for periodic flush of buffered trace data */
int trace_init( const char *trace_dir, const uint32_t trace_id )
{
    return 0;
}

/* Complete tracing, flush buffers and close files, terminate the captive thread */
int trace_fini( void )
{
    return 0;
}

/* Record timestamp and elapsed delta for a write IOP */
int trace_write( const struct timespec *ts, const struct timespec *iop )
{
    return 0;
}

/* Record timestamp and elapsed delta for a read IOP */
int trace_read( const struct timespec *ts, const struct timespec *iop )
{
    return 0;
}
