obj-m += nf_conntrack_dummy.o

EXTRA_CFLAGS += -g -Wall

all: rdummy sdummy

dummy.o: dummy.c dummy.h
	cc ${EXTRA_CFLAGS} -c dummy.c

sdummy.o: sdummy.c
	cc ${EXTRA_CFLAGS} -c $?

rdummy.o: rdummy.c
	cc ${EXTRA_CFLAGS} -c $?

sdummy: sdummy.o dummy.o
	cc -o $@ sdummy.o dummy.o

rdummy: rdummy.o dummy.o
	cc -o $@ rdummy.o dummy.o



