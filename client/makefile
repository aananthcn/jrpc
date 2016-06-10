.DEFAULT_GOAL := all

# constants
IFLAGS = -I. 

CFLAGS = -g -fPIC -Wall -Werror ${IFLAGS}
LFLAGS = -g -shared -lpthread -ljansson

MKDIR  = mkdir -p

TARGET = ../bin/libjrpc.so

# objects
objs = ejson.o \
       jrpc.o 



%.o: %.c
	$(CC) -c $(CFLAGS) $^ -o $@



shared_object: ${objs}
	$(CC) -o ${TARGET} $^ $(LFLAGS)


clean:
	$(RM) ${objs} 
	$(RM) ${TARGET}


all: shared_object

