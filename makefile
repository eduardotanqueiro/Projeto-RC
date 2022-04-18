FLAGS = -Wall -g -pthread
CC = gcc


PROG = server
OBJS = server.o admin.o client.o


all: ${PROG} clean

clean:
		rm -f ${OBJS}
		

${PROG}:	${OBJS}
		${CC} ${FLAGS} ${OBJS} -o $@

.c.o:
		${CC} ${FLAGS} $< -c

##################


server.o: server.c header.h

admin.o: admin.c header.h

client.o: client.c header.h


