/*------------------------------------------------------------------------------------------------*/
/* Emitting performance traces.
 * Generating telemetry streams during benchmark execution */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include "utils.h"
#include "pthread.h"

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

#define TRACE_MAXENT 		65536		/* Must fit in uint16_t */
#define TRACE_NENT   		TRACE_MAXENT 	/* TODO - paramterize   */
#define TRACE_MOD_ADD(x, y) 	((unsigned)((x)+(y)) % TRACE_NENT)
#define TRACE_MOD_INC(x)    	TRACE_MOD_ADD((x), 1)


/* Trace buffer management strutures  */
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

/* File write control */
#define TRACE_WRITE_SIZE 	8192	
#define TRACE_BLOCK 		(TRACE_WRITE_SIZE/sizeof(trace_entry_t)) 

void *trace_sync ( void );	/* Flush thread */

const char *trace_type_str( const trace_type_t T )
{
    static const struct { trace_type_t T; const char *str; } table[] =
    {
	{ TRACE_READ, "READ" },
	{ TRACE_WRITE, "WRITE" },
	{ TRACE_MISC, "MISC" },
    };
    for( unsigned i=0; i < ARRAYLEN(table); i++ )
    {
	if( table[i].T == T )	return table[i].str;
    }
    return "????";
}

/* 
 * Initialize trace state. This includes creation of trace log file and 
 * spawn of trace buffer flush thread.
 */
int trace_init( const char *trace_dir, const uint32_t trace_id )
{
    char path[PATH_MAX]; 
    int ret;

    log_debug( "trace_init" );

    /* Create handle to trace log file */
    sprintf(path, "%s/%x.trc", trace_dir, trace_id);
    log_debug( "trace_file: %s", path );
    if ( (ti.ti_fp=fopen(path, "w+")) == NULL ) {
        log_error("open of trace file (%s) failed, (%d) ", path, errno);
        return( -1 );
    }

    /* Initialize trace state */
    ti.ti_lastflush = ti.ti_nextflush = (uint16_t)(TRACE_NENT-1);
    ti.ti_nextent = 0;
    ti.ti_req = TRACE_NONE;

    /* Spawn flush thread */
    log_debug( "spawn" );
    if ( (ret = pthread_create( &ti.ti_flushthread, NULL, (void*)trace_sync, 
                                (void*)&ti )) < 0 ) {
        log_error( "pthread_create failed - %d", ret );
        return -1;
    }
    
    log_debug( "trace_init return" );
    return 0;
}

/* Complete tracing, flush buffers and close files, terminate the captive thread */
int trace_fini( void )
{
    log_debug( "trace_fini" );

    /* Send final flush / exit request to trace thread */
    pthread_mutex_lock( &trace_flush_mutex );
    log_debug( "trigger flush at %d", ti.ti_nextent-1 );
    ti.ti_req = TRACE_EXIT;
    ti.ti_nextflush = ti.ti_nextent-1;
    pthread_cond_signal( &trace_cond );
    pthread_mutex_unlock( &trace_flush_mutex );

    /* Wait for logging thread to terminate */
    log_debug( "wait for thread to terminate" );
    pthread_join( ti.ti_flushthread, NULL );
    return 0;
}

/* Utility function to request trace buffer flush */
void trace_flush()
{
    log_debug( "trace_flush" );

    /* Send flush request to logging thread */
    pthread_mutex_lock( &trace_flush_mutex );
    ti.ti_req = TRACE_FLUSH;
    pthread_cond_signal( &trace_cond );
    pthread_mutex_unlock( &trace_flush_mutex );
}

/* 
 * Create trace entry. Flush outstanding trace entries periodically 
 * (based on TRACE_BLOCK) 
 */
int trace( const trace_type_t tt, const struct timespec *ts, 
           const struct timespec *iop, const char *tag )
{
    uint16_t next = ti.ti_nextent;
    trace_entry_t *te = &ti.ti_tracebuf[ti.ti_nextent];

    ti.ti_nextent = TRACE_MOD_INC(ti.ti_nextent);

    log_debug( "got trace request %d", next );

    /* Create trace entry */
    te->info.op = tt;
    te->timestamp = *ts;
    te->duration = *iop;

    /* Add tag to entry if appropriate */
    if ( tag ) {
        strncpy(&te->info.tag[0], tag, 7);
    } else {
        te->info.tag[0] = 0;
    }

    /* Request buffer flush if enough entries have accumulated */
    if (( ti.ti_nextent % TRACE_BLOCK) == 0 ) {
        log_debug( "trigger flush at %d", ti.ti_nextent-1 );
        pthread_mutex_lock( &trace_flush_mutex );
        ti.ti_req = TRACE_FLUSH;
        ti.ti_nextflush = te - &ti.ti_tracebuf[0];
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
void *trace_sync ( )
{
    log_debug( "in thread" );
    
    for (;;) {
        trace_req_t req;
        trace_entry_t *tbp;
        uint16_t lastflush;
        uint16_t nextflush;

        /* Wait for dump request */
        pthread_mutex_lock( &trace_flush_mutex );
        log_debug( "wait for req - %d", ti.ti_req );
        req = ti.ti_req;
        lastflush = ti.ti_lastflush;
        nextflush = ti.ti_nextflush;

        /* 
         * If no operation request pending, wait for request to arrive. 
         * Otherwise, we update the buffer pointers to determine flush 
         * range while we have the lock.
         */
        ti.ti_req = TRACE_NONE;
        if ( req == TRACE_NONE ) {
            pthread_cond_wait( &trace_cond, &trace_flush_mutex );
        } 
        pthread_mutex_unlock( &trace_flush_mutex );
        switch (req) {
            int thisflush;
            uint16_t nflush;

            case TRACE_EXIT:
            case TRACE_FLUSH:
                thisflush = TRACE_MOD_INC(lastflush);

                log_debug( "got flush request %hu<-->%hu", 
                            thisflush, nextflush );
                tbp = &ti.ti_tracebuf[thisflush];

                /* Handle wrap */
                if ( thisflush > nextflush ) 
                {
                    nflush = TRACE_NENT - thisflush;
                    ti.ti_lastflush = TRACE_MOD_ADD(ti.ti_lastflush, nflush);

                    log_debug( "writing %hu records from %hu", 
                                nflush, thisflush);
                    if (nflush && (nflush != 
                                   fwrite( tbp, sizeof(trace_entry_t), 
                                          nflush, ti.ti_fp ))) 
                    {
                        log_error( "trace buffer write failed - %d", errno );
                        return (void*)-1;
                    }
                    tbp = &ti.ti_tracebuf[0];
                    nflush = nextflush + 1;
                    thisflush = 0;
                } else {
                    nflush = (nextflush - thisflush) + 1;
                }

                ti.ti_lastflush = TRACE_MOD_ADD(ti.ti_lastflush, nflush);

                /* Flush any remaining entries */
                log_debug( "writing %hu records from %d", nflush, thisflush);
                if (nflush && (nflush != fwrite( tbp, sizeof(trace_entry_t), 
                                      nflush, ti.ti_fp )))
                {
                    log_error( "trace buffer write failed - %d", errno );
                    return (void*)-1;
                }

                /* Terminate if requested */
                if ( req == TRACE_EXIT ) {
                    log_debug( "got exit request" );
                    fclose( ti.ti_fp );
                    return (void*)0;
                }
                break;
            default:
		log_debug( "unknown req %d", req);
                break;
        }
     }
}
