CC = gcc
CFLAGS = -g -std=c99
CPPFLAGS = -Iinclude
LDFLAGS =
LIBS = -lpthread -lm

PRNG_SRCS = prng/prng.c prng/prng_debug.c prng/prng_xorshift.c
PRNG_OBJS = $(PRNG_SRCS:%.c=%.o)

SAMPLE_SRCS = sample/sample.c sample/sample_debug.c
SAMPLE_OBJS = $(SAMPLE_SRCS:%.c=%.o)

MOTIF_1_SRCS = motif_1.c
MOTIF_1_OBJS = $(MOTIF_1_SRCS:%.c=%.o) 

motif_1: $(MOTIF_1_OBJS) $(PRNG_OBJS) $(SAMPLE_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(MOTIF_1_OBJS) $(PRNG_OBJS) $(SAMPLE_OBJS) $(LIBS)

.PHONY: clean

clean: 
	$(RM) motif_1 $(MOTIF_1_OBJS) $(PRNG_OBJS)
