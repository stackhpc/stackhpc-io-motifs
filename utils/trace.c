/*------------------------------------------------------------------------------------------------*/
/* Emitting performance traces.
 * Generating telemetry streams during benchmark execution */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "utils.h"
#include "pthread.h"

extern int errno ;


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


#define TRACE_NENT 16384 	/* TODO - paramterize the size (trace_init) */

typedef enum trace_req {
    TRACE_NONE = 0,		/* No pending operation */
    TRACE_FLUSH,		/* Flush the current trace buffer */
    TRACE_EXIT			/* Flush trace buffer and exit */
} trace_req_t;
  
typedef struct traceinfo {
    pthread_t ti_flushthread;	/* Handle for buffer flush thread */
    trace_req_t ti_req;		/* Current pending flush operationn */
    FILE *ti_fp;		/* Output file pointer */
    uint16_t ti_nextent;	/* Next trace entry to fill */
    uint16_t ti_lastflush;	/* Last flushed entry (head pointer) */
    uint16_t ti_nextflush;	/* Next flush marker (tail pointer */
    trace_entry_t ti_tracebuf[TRACE_NENT];
} trace_info_t;

static trace_info_t ti;		/* Active trace state */

/* Synchronization structures to manage flush thread */
static pthread_mutex_t trace_flush_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  trace_cond        = PTHREAD_COND_INITIALIZER;

#define TRACE_BLOCK 100 	/* Blocking factor for buffer flushing */

void *sync_trace ( void );	/* Flush thread */

/* 
 * Initialize trace state. This includes creation of trace log file and 
 * spawn of trace buffer flush thread.
 */
int trace_init( const char *trace_dir, const uint32_t trace_id )
{
    char path[1024]; 
    int ret;

    log_info( "trace_init" );

    /* Create handle to trace log file */
    sprintf(path, "%s/%x.trc", trace_dir, trace_id);
    log_info( "trace_file: %s", path );
    if ( (ti.ti_fp=fopen(path, "w+")) == NULL ) {
        log_error("open of trace file (%s) failed, (%d) ", path, errno);
        return( -1 );
    }

    /* Initialize trace state */
    ti.ti_lastflush = 0;
    ti.ti_nextflush = 0;
    ti.ti_nextent = 0;
    ti.ti_req = TRACE_NONE;

    /* Spawn flush thread */
    log_info( "spawn" );
    if ( (ret = pthread_create( &ti.ti_flushthread, NULL, (void*)sync_trace, (void*)&ti )) < 0 ) {
        log_error( "pthread_create failed - %d", ret );
        return -1;
    }
    
    log_info( "trace_init return" );
    return 0;
}

/* Complete tracing, flush buffers and close files, terminate the captive thread */
int trace_fini( void )
{
    log_info( "trace_fini" );

    /* Send final flush / exit request to trace thread */
    pthread_mutex_lock( &trace_flush_mutex );
    ti.ti_req = TRACE_EXIT;
    pthread_cond_signal( &trace_cond );
    pthread_mutex_unlock( &trace_flush_mutex );

    /* Wait for logging thread to terminate */
    log_info( "wait for thread to terminate" );
    pthread_join( ti.ti_flushthread, NULL );
    return 0;
}

/* Utility function to request trace buffer flush */
void trace_flush()
{
    log_info( "trace_flush" );

    /* Send flush request to logging thread */
    pthread_mutex_lock( &trace_flush_mutex );
    ti.ti_req = TRACE_FLUSH;
    pthread_cond_signal( &trace_cond );
    pthread_mutex_unlock( &trace_flush_mutex );
}

/* Create trace entry. Flush outstanding trace entries periodically (based on TRACE_BLOCK) */
int trace( const trace_type_t tt, const struct timespec *ts, const struct timespec *iop )
{
    trace_entry_t *te = &ti.ti_tracebuf[ti.ti_nextent++];

    log_info( "got trace request" );

    /* Create trace entry */
    te->info.op = tt;
    te->info.tag = 0;
    te->timestamp = *ts;
    te->duration = *iop;

    /* Request buffer flush if enough entries have accumulated */
    if (( ti.ti_nextent % TRACE_BLOCK) == 0 ) {
        log_info( "trigger flush at %d", ti.ti_nextent-1 );
        pthread_mutex_lock( &trace_flush_mutex );
        ti.ti_nextflush = te - &ti.ti_tracebuf[0];
        ti.ti_req = TRACE_FLUSH;
        pthread_cond_signal( &trace_cond );
        pthread_mutex_unlock( &trace_flush_mutex );
    }

    return 0;
}

/* 
 * Trace persistence thread. The thread waits for flush requests from the main
 * task and handles them accordingly. If an exit request is observed, the thread
 * will flush any outstanding trace data and terminate.
 */
void *sync_trace ( )
{
    log_info( "in thread" );
    
    for (;;) {
        trace_req_t req;
        trace_entry_t *tbp;
        uint16_t lastflush;
        uint16_t nextflush;
        uint16_t nflush;

        /* Wait for dump request */
        pthread_mutex_lock( &trace_flush_mutex );
        log_info( "wait for req - %d", ti.ti_req );
        req = ti.ti_req;
        lastflush = ti.ti_lastflush;
        nextflush = ti.ti_nextflush;
        nflush = nextflush - lastflush;

        /* 
         * If no operation request pending, wait for request to arrive. Otherwise,
         * we update the buffer pointers to determine flush range while we have
         * the lock.
         */
        ti.ti_req = TRACE_NONE;
        if ( req == TRACE_NONE ) {
            pthread_cond_wait( &trace_cond, &trace_flush_mutex );
        } else {
            ti.ti_lastflush = nextflush+1;
        }
        pthread_mutex_unlock( &trace_flush_mutex );
        switch (req) {
            case TRACE_EXIT:
            case TRACE_FLUSH:
                /* Flush the end of the buffer if we've wrapped. */
                log_info( "got flush request %d<-->%d", lastflush, nextflush );
                tbp = &ti.ti_tracebuf[lastflush];
                if ( lastflush > nextflush ) 
                {
                    int nflush = TRACE_NENT - lastflush;
                    if (nflush != fwrite( tbp, sizeof(trace_entry_t), nflush, ti.ti_fp )) 
                    {
                        log_error( "trace buffer write failed - %d", errno );
                        return (void*)-1;
                    }
                    tbp = &ti.ti_tracebuf[0];
                    lastflush = 0;
                    nflush = nextflush;
                }

                /* Flush any remaining entries */
                if (nflush != fwrite( tbp, sizeof(trace_entry_t), nflush, ti.ti_fp ))
                {
                    log_error( "trace buffer write failed - %d", errno );
                    return (void*)-1;
                }

                /* Terminate if requested */
                if ( req == TRACE_EXIT ) {
                    log_info( "got exit request" );
                    fclose( ti.ti_fp );
                    return (void*)0;
                }
                break;
            default:
		log_info( "unknown req %d", req);
                break;
        }
     }
}
