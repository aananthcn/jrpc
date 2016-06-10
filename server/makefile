.DEFAULT_GOAL := all

# constants
IFLAGS = -I. 

CFLAGS = -g -Wall ${IFLAGS}
#CFLAGS = -g -Wall -Werror ${IFLAGS}
LFLAGS = -g -lpthread -ljansson

MKDIR  = mkdir -p

TARGET = ../bin/jrpcd

# objects
objs = jrpcd.o  \
       jrpcd_client.o  \
       jrpcd_parser.o  \
       jrpcd_queue.o  \
       jrpcd_server.o  \
       main.o



%.o: %.c
	$(CC) -c $(CFLAGS) $^ -o $@



executable: ${objs}
	$(CC) -o ${TARGET} $^ $(LFLAGS)


clean:
	$(RM) ${objs} 
	$(RM) ${TARGET}


all: executable

