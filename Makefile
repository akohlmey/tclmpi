#!/usr/bin/make
SHELL=/bin/sh
# please adjust to your local setup
########### configuration section ###########
# MPI C-compiler wrapper
CC=mpicc

# linker (usually the same as compiler)
LD=$(CC)

# set to empty if you don't want to include debug info
DEBUG=-g

# platform specific compiler flags:
## Linux x86 32-bit:
#COMPILE=-fPIC -m32 -O2 -Wall -W
#LINK=-shared -m32
## Linux x86 64-bit:
COMPILE=-fPIC -m64 -O2 -Wall -W
LINK=-shared -m64
## MacOSX x86 32-bit
#COMPILE=-Os -Wall -m32 -fPIC -dynamic
#LINK= -bundle -m32 -L/usr/X11R6/lib/
## MacOSX x86 64-bit
#COMPILE=-Os -Wall -fPIC -dynamic -m64
#LINK= -bundle -L/usr/X11R6/lib/
## Win32/MinGW via cross compiler
#CC=i686-pc-mingw32-gcc
#COMPILE= -DMPIWRAPSTCLDLL_EXPORTS -O2 -Wall -W -fno-strict-aliasing
#LINK= -shared 

# set, if needed to match Tcl installation
#TCLINCLUDE=-I/usr/include
#TCLLIBRARY=-L/usr/lib 
#TCLLIB=-ltcl

# set, if needed, to match MPI installation
# not needed if MPI compiler wrappers work
#MPIINCLUDE=-I/usr/include
#MPILIBRARY=-L/usr/lib
#MPILIB=-lmpi
######## end of configuration section #######

CFLAGS=$(COMPILE) $(DEBUG) $(TCLINCLUDE) $(MPIINCLUDE) 
LDFLAGS=$(LINK) $(DEBUG) $(TCLLIBRARY) $(MPILIBRARY)
LIBS= $(TCLLIB) $(MPILIB)

default: tclmpi.so

clean:
	rm -f tclmpi.so *.o *~

#############################################
tclmpi.so:  tcl_mpi.o
	$(LD) $(LDFLAGS) -o tclmpi.so $^ $(LIBS)

tcl_mpi.o: tcl_mpi.c
	$(CC) $(CFLAGS) -c $<
