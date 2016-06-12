.DEFAULT_GOAL := all

# constants
IFLAGS = -I. 

CFLAGS = -Wall -Werror ${IFLAGS}
LFLAGS = -lpthread -ljansson

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


debug: CFLAGS += -g -DDEBUG -Wall -Werror ${IFLAGS}
debug: LFLAGS += -g -DDEBUG -lpthread -ljansson
debug: executable


all: executable


