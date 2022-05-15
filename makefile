FLAGS = -Wall -g -pthread
CC = gcc


PROG1 = server
OBJS1 = server.o admin.o client.o

PROG2 = client
OBJS2 = client_program.o

all: ${PROG1} clean1
all: ${PROG2} clean2

clean1:
		rm -f ${OBJS1}
clean2:
		rm -f ${OBJS2}

${PROG1}:	${OBJS1}
		${CC} ${FLAGS} ${OBJS1} -o $@

${PROG2}:	${OBJS2}
		${CC} ${FLAGS} ${OBJS2} -o $@

.c.o:
		${CC} ${FLAGS} $< -c

##################


server.o: server.c header.h

admin.o: admin.c header.h

client.o: client.c header.h

client_program.o: client_program.c header.h client_program.h


