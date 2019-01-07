CC = gcc
CFLAGS = -g -std=c99
CPPFLAGS = -Iinclude
LDFLAGS =
LIBS = -lpthread -lm

PRNG_SRCS = prng/prng.c prng/prng_debug.c
PRNG_OBJS = $(PRNG_SRCS:%.c=%.o)

MOTIF_1_SRCS = motif_1.c
MOTIF_1_OBJS = $(MOTIF_1_SRCS:%.c=%.o) 

motif_1: $(MOTIF_1_OBJS) $(PRNG_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(MOTIF_1_OBJS) $(PRNG_OBJS) $(LIBS)

clean: motif_1 $(MOTIF_1_OBJS) $(PRNG_OBJS)
	$(RM) $^
