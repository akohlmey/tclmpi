#!/usr/bin/make
SHELL=/bin/sh
VERSION=0.7
# please adjust to your local setup
########### configuration section ###########
# MPI C-compiler wrapper
CC=mpicc

# linker (usually the same as compiler)
LD=$(CC)

# set to empty if you don't want to include debug info
DEBUG=-g

# comment out if you want to use 
STUBS=-DUSE_TCL_STUBS

# defines
DEFINE=-DPACKAGE_NAME=\"_tclmpi\" -DPACKAGE_VERSION=\"$(VERSION)\"

# platform specific compiler flags:
## Linux and multiple other platforms with GCC (generic)
COMPILE=-fPIC  -O2 -Wall -W
DYNLINK=-shared 
## Linux x86 32-bit:
#COMPILE=-fPIC -m32 -O2 -Wall -W
#DYNLINK=-shared -m32
## Linux x86 64-bit:
#COMPILE=-fPIC -m64 -O2 -Wall -W
#DYNLINK=-shared -m64
## MacOSX x86 32-bit
#COMPILE=-Os -Wall -m32 -fPIC -dynamic
#DYNLINK= -bundle -m32 -L/usr/X11R6/lib/
## MacOSX x86 64-bit
#COMPILE=-Os -Wall -fPIC -dynamic -m64
#DYNLINK= -bundle -L/usr/X11R6/lib/
## Win32/MinGW via cross compiler
#CC=i686-pc-mingw32-gcc
#COMPILE= -DMPIWRAPSTCLDLL_EXPORTS -O2 -Wall -W -fno-strict-aliasing
#DYNLINK= -shared 

# set, if needed to match Tcl installation
TCLINCLUDE=-I/usr/include
TCLLIBRARY=-L/usr/lib64 -L/usr/lib
TCLSTUBLIB=-ltclstub8.5
TCLLIB=-ltcl8.5

# set, if needed, to match MPI installation
# not needed if MPI compiler wrappers work
#MPIINCLUDE=-I/usr/include
#MPILIBRARY=-L/usr/lib
#MPILIB=-lmpi
######## end of configuration section #######

CFLAGS=$(COMPILE) $(DEBUG) $(DEFINE) $(TCLINCLUDE) $(MPIINCLUDE) 
LDFLAGS=$(DEBUG) $(TCLLIBRARY) $(MPILIBRARY)
DYNLIBS= $(TCLSTUBLIB) $(MPILIB)
LIBS= $(TCLLIB) $(MPILIB)

default: version _tclmpi.so

dynamic: version _tclmpi.so check
static: version tclmpish check-static
all: dynamic static doc

clean:
	rm -f _tclmpi.so tclmpish *.o *~ tests/*~ examples/*~
	rm -rf docs/* doxygen.log
	rm -f pkgIndex.tcl Doxyfile tclmpi.tcl

check: version _tclmpi.so
	(cd tests; ./test_01.tcl)
	(cd tests; ./test_02.tcl)
	(cd tests; mpirun -np 2 ./test_03.tcl)
	(cd tests; mpirun -np 2 ./test_04.tcl)

check-static: version tclmpish
	(cd tests; ../tclmpish ./test_01.tcl)
	(cd tests; ../tclmpish ./test_02.tcl)
	(cd tests; mpirun -np 2 ../tclmpish ./test_03.tcl)
	(cd tests; mpirun -np 2 ../tclmpish ./test_04.tcl)

#############################################
_tclmpi.so:  _tclmpi.o
	$(LD) $(DYNLINK) $(LDFLAGS) -o $@ $^ $(DYNLIBS)

_tclmpi.o: _tclmpi.c
	$(CC) $(CFLAGS) $(STUBS) -c $<

tclmpish: _tclmpi.c
	$(LD) $(CFLAGS) -DBUILD_TCLMPISH $< -o $@ $(LDFLAGS) $(LIBS)

#############################################

doc: refman.pdf

refman.pdf: Doxyfile _tclmpi.c tests/harness.tcl tclmpi.tcl docs
	doxygen || rm -f $@
	make -C docs/latex || rm -f $@
	cp -p docs/latex/refman.pdf $@ || rm -f $@

docs:
	mkdir docs

#############################################
version: Doxyfile tclmpi.tcl pkgIndex.tcl

Doxyfile: Doxyfile.in
	sed -e s,@VERSION@,$(VERSION),g $< > $@

tclmpi.tcl: tclmpi.tcl.in pkgIndex.tcl
	sed -e s,@VERSION@,$(VERSION),g $< > $@

pkgIndex.tcl: pkgIndex.tcl.in
	sed -e s,@VERSION@,$(VERSION),g $< > $@

#############################################
.PHONY: default clean check doc all dynamic static check-static version
.SUFFIXES:

