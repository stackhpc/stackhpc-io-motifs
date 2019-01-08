CC = gcc
CFLAGS = -g -std=c99
CPPFLAGS = -Iinclude
LDFLAGS =
LIBS = -lpthread -lm

COMMON_SRCS = prng/prng.c prng/prng_debug.c prng/prng_xorshift.c \
              sample/sample.c sample/sample_debug.c \
              storage/storage.c storage/storage_debug.c \
              log/log.c utils/time.c utils/trace.c

COMMON_OBJS = $(COMMON_SRCS:%.c=%.o)

MOTIF_1_SRCS = motif_1.c
MOTIF_1_OBJS = $(MOTIF_1_SRCS:%.c=%.o) 

motif_1: $(MOTIF_1_OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(MOTIF_1_OBJS) $(COMMON_OBJS) $(LIBS)

.PHONY: clean

clean: 
	$(RM) motif_1 $(MOTIF_1_OBJS) $(COMMON_OBJS)
