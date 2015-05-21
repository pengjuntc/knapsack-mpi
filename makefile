CC=gcc
MPICC=mpicc
CFLAG=-g
targets = mpi knap
objects = mpi.o knap.o

.PHONY: default
default: all

.PHONY: all
all: $(targets)

knap: knap.c
	$(CC) $(CFLAG) -o $@ $^
mpi: knap_mpi.c
	$(MPICC) $(CFLAG) -o $@ $^
clean:
	rm -f *~ $(targets) $(objects)
