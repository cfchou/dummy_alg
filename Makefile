obj-m += nf_conntrack_dummy.o

CFLAGS+=-g -Wall

all: rdummy sdummy

dummy.o: dummy.c dummy.h
	cc ${CFLAGS} -c dummy.c

sdummy.o: sdummy.c
	cc ${CFLAGS} -c $?

rdummy.o: rdummy.c
	cc ${CFLAGS} -c $?

sdummy: sdummy.o dummy.o
	cc -o $@ sdummy.o dummy.o

rdummy: rdummy.o dummy.o
	cc -o $@ rdummy.o dummy.o



