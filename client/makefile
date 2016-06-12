.DEFAULT_GOAL := all

# constants
IFLAGS = -I. -I../server

CFLAGS = -fPIC -Wall -Werror ${IFLAGS}
LFLAGS = -shared -lpthread -ljansson

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


debug: CFLAGS = -g -DDEBUG -fPIC -Wall -Werror ${IFLAGS}
debug: LFLAGS = -g -DDEBUG -shared -lpthread -ljansson 
debug: shared_object 


all: shared_object

