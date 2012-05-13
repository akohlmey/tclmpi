# configuration
CC=mpicc
LD=$(CC)
CFLAGS=-fPIC -O2 -Wall -W
LDFLAGS=-shared
LIBS=-ltcl
#############################################

default: tclmpi.so

clean:
	rm -f tclmpi.so *.o *~

#############################################
tclmpi.so:  tcl_mpi.o
	$(LD) $(LDFLAGS) -o tclmpi.so $^ $(LIBS)

tcl_mpi.o: tcl_mpi.c
	$(CC) $(CFLAGS) -c $<
