CFLAGS = -g
CPPFLAGS = 
LDFLAGS =
LIBS = -lpthread -lm

MOTIF_1_SRCS = motif_1.c
MOTIF_1_OBJS = $(MOTIF_1_SRCS:%.c=%.o) 

motif_1: $(MOTIF_1_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(MOTIF_1_OBJS) $(LIBS)

clean: motif_1 $(MOTIF_1_OBJS)
	$(RM) $^
