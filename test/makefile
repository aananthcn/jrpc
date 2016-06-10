.DEFAULT_GOAL := all

# constants
IFLAGS = -I. -I../server -I../client

CFLAGS = -g ${IFLAGS}
LFLAGS = -lpthread -ljansson -ljrpc -L../bin

MKDIR  = mkdir -p


# objects
sum_objs = sum.o

avg_objs = average.o



%.o: %.c
	$(CC) -c $(CFLAGS) $^ -o $@



sum: ${sum_objs}
	$(CC) -o $@ $^ $(CFLAGS) $(LFLAGS)
	mv $@ ../bin/


average: ${avg_objs}
	$(CC) -o $@ $^ $(CFLAGS) $(LFLAGS)
	mv $@ ../bin/


clean:
	$(RM) ${sum_objs} 
	$(RM) ${avg_objs} 
	$(RM) ../bin/sum ../bin/average


all: sum average

