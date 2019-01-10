/*------------------------------------------------------------------------------------------------*/
/* Interpret trace file contents */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include "utils.h"

trace_entry_t tracebuf[100];
char *trace_op_by_name[] = {"READ", "WRITE", "MISC"};
#define TRACE_OP_NAME(x) trace_op_by_name[(x)]

typedef enum {
    TEXT_MODE = 0,
    CSV_MODE
} output_mode_t;

/* Output trace in human readable format */
void print_trace( trace_entry_t *tp )
{
    char buf[8];
    char *tbp = &tp->info.tag[0];
    buf[7] = 0;

    strncpy(buf, tbp, sizeof(tp->info.tag));

    printf( "timestamp:%d.%d, duration:%d.%d, operation:%s, tag:%s\n", 
           (unsigned)tp->timestamp.tv_sec,
           (unsigned)tp->timestamp.tv_nsec,
           (unsigned)tp->duration.tv_sec,
           (unsigned)tp->duration.tv_nsec,
           TRACE_OP_NAME(tp->info.op),
           buf);
}

/* Output trace in csv format */
void csv_trace( trace_entry_t *tp )
{
    char buf[8];
    char *tbp = &tp->info.tag[0];
    buf[7] = 0;

    strncpy(buf, tbp, sizeof(tp->info.tag));

    printf( "%d.%d,%d.%d,%s,%s\n", 
           (unsigned)tp->timestamp.tv_sec,
           (unsigned)tp->timestamp.tv_nsec,
           (unsigned)tp->duration.tv_sec,
           (unsigned)tp->duration.tv_nsec,
           TRACE_OP_NAME(tp->info.op),
           buf);
}

/* Handle argument parsing error */
void fail( char *cmd )
{
    fprintf( stderr, "Usage: %s [-c] [-t] <path_to_file>\n\n", cmd );
    fprintf( stderr, "\t-c Output in CSV format\n" );
    fprintf( stderr, "\t-t Output in text format\n" );
    exit( 1 );
}

/* Read trace file and output in desired format */
int main( int argc, char **argv )
{
    FILE *fp;
    output_mode_t m = TEXT_MODE;
    char *file = NULL;
    int c, nent;

    opterr = 0;
    
    while (( c = getopt (argc, argv, "ct" )) != -1 ) {
        switch (c)
        {
        case 'c':
            m = CSV_MODE;
            break;
        case 't':
            m = TEXT_MODE;
            break;
        default:
           fail(argv[0]);
        }
    }

    if ( optind == (argc - 1) ) {
        file = argv[ optind ];
    } else {
        fail(argv[0]);
    }

    if ( (fp=fopen(file, "r")) == NULL ) {
        log_error("open of trace file (%s) failed, (%d) ", file, errno);
        return( -1 );
    }

    while ((nent = fread( &tracebuf[0], sizeof(trace_entry_t), 100, fp ))) {
        for (int i=0; i<nent; i++) {
            if ( m == TEXT_MODE )
                print_trace( &tracebuf[i] );
            else 
                csv_trace( &tracebuf[i] );
        }
    }
}
