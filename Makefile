CC = gcc
CFLAGS = -Wall -g -std=c99
CPPFLAGS = -Iinclude -D_POSIX_C_SOURCE=200809L
LDFLAGS = -lrt
LIBS = -lrados -lpthread -lm

MAJOR = 0
MINOR = 1
BUILD_DATE = $(shell date +"%Y%m%d.%H%M")
VERSION= "\"$(MAJOR).$(MINOR).$(BUILD_DATE)\""

CPPFLAGS += -DVERSION=$(VERSION)

COMMON_SRCS = prng/prng.c prng/prng_debug.c prng/prng_xorshift.c \
              sample/sample.c sample/sample_debug.c \
              storage/storage.c storage/storage_debug.c storage/storage_dirtree.c storage/storage_rados.c \
              log/log.c utils/time.c utils/trace.c utils/barrier.c

UTILS = utils/tracefmt

TESTS = test/test_log test/test_prng test/test_trace test/test_sample test/test_storage test/test_rados

COMMON_OBJS = $(COMMON_SRCS:%.c=%.o)

MOTIF_1_SRCS = motif_1.c
MOTIF_1_OBJS = $(MOTIF_1_SRCS:%.c=%.o)

motif_1: $(MOTIF_1_OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(MOTIF_1_OBJS) $(COMMON_OBJS) $(LIBS)


TRACE_READ_SRCS = trace_read.c
TRACE_READ_OBJS = $(TRACE_READ_SRCS:%.c=%.o)

trace_read: $(TRACE_READ_OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(TRACE_READ_OBJS) $(COMMON_OBJS) $(LIBS)


tests: $(TESTS)

test/test_log test/test_prng test/test_trace test/test_sample test/test_storage test/test_rados: $(COMMON_OBJS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $@.c $(COMMON_OBJS) $(LIBS)

utils: $(UTILS)

utils/tracefmt: $(COMMON_OBJS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $@.c $(COMMON_OBJS) $(LIBS)

.PHONY: clean

clean:
	$(RM) motif_1 $(MOTIF_1_OBJS) $(COMMON_OBJS) $(TEST_OBJS) $(TESTS) test/*.trc $(UTILS)
