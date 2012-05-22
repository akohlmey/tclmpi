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

# defines
DEFINE=-DUSE_TCL_STUBS -DPACKAGE_NAME=\"_tclmpi\" -DPACKAGE_VERSION=\"0.6\"

# platform specific compiler flags:
## Linux and multiple other platforms with GCC (generic)
COMPILE=-fPIC  -O2 -Wall -W
LINK=-shared 
## Linux x86 32-bit:
#COMPILE=-fPIC -m32 -O2 -Wall -W
#LINK=-shared -m32
## Linux x86 64-bit:
#COMPILE=-fPIC -m64 -O2 -Wall -W
#LINK=-shared -m64
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
TCLINCLUDE=-I/usr/include
TCLLIBRARY=-L/usr/lib64 -L/usr/lib
TCLLIB=-ltclstub8.5

# set, if needed, to match MPI installation
# not needed if MPI compiler wrappers work
#MPIINCLUDE=-I/usr/include
#MPILIBRARY=-L/usr/lib
#MPILIB=-lmpi
######## end of configuration section #######

CFLAGS=$(COMPILE) $(DEBUG) $(DEFINE) $(TCLINCLUDE) $(MPIINCLUDE) 
LDFLAGS=$(LINK) $(DEBUG) $(TCLLIBRARY) $(MPILIBRARY)
LIBS= $(TCLLIB) $(MPILIB)

default: _tclmpi.so

all: _tclmpi.so refman.pdf check

clean:
	rm -f _tclmpi.so *.o *~ tests/*~ examples/*~
	rm -rf docs/*

check: _tclmpi.so
	(cd tests; ./test_01.tcl)
	(cd tests; ./test_02.tcl)
	(cd tests; mpirun -np 2 ./test_03.tcl)
	(cd tests; mpirun -np 2 ./test_04.tcl)

#############################################
_tclmpi.so:  _tclmpi.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

_tclmpi.o: _tclmpi.c
	$(CC) $(CFLAGS) -c $<

#############################################

doc: refman.pdf

refman.pdf: Doxyfile _tclmpi.c tests/harness.tcl tclmpi.tcl docs
	doxygen || rm -f $@
	make -C docs/latex || rm -f $@
	cp -p docs/latex/refman.pdf $@ || rm -f $@

docs:
	mkdir docs

#############################################
.PHONY: default clean check doc all
.SUFFIXES:

