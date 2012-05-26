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
# comment out if you want to use the stubs interface
STUBS=-DUSE_TCL_STUBS

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
TCLLIB=-ltcl8.5 -dl

# set, if needed, to match MPI installation
# not needed if MPI compiler wrappers work
#MPIINCLUDE=-I/usr/include
#MPILIBRARY=-L/usr/lib
#MPILIB=-lmpi
######## end of configuration section #######
NAME=tclmpi
# defines
DEFINE=-DPACKAGE_NAME=\"_$(NAME)\" -DPACKAGE_VERSION=\"$(VERSION)\" \
	-DMPICH_SKIP_MPICXX -DOMPI_SKIP_MPICXX=1

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
	rm -rf latex doxygen.log
	rm -f pkgIndex.tcl Doxyfile tclmpi.tcl
	rm -f tclmpi-*.tar.gz tclmpi-*.pdf

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

doc: $(NAME)-$(VERSION)-docs.pdf

$(NAME)-$(VERSION)-docs.pdf: Doxyfile _tclmpi.c \
	tests/harness.tcl tclmpi.tcl
	doxygen || rm -f $@
	make -C latex || rm -f $@
	cp -p latex/refman.pdf $@ || rm -f $@

tar: doc
	rm -rvf $(NAME)-$(VERSION)
	mkdir $(NAME)-$(VERSION)
	cp $(NAME)-$(VERSION)-docs.pdf $(NAME)-$(VERSION)/
	cp Makefile *.in _tclmpi.c README INSTALL LICENSE $(NAME)-$(VERSION)
	mkdir $(NAME)-$(VERSION)/tests
	cp tests/README $(NAME)-$(VERSION)/tests
	cp tests/*.tcl $(NAME)-$(VERSION)/tests
	mkdir $(NAME)-$(VERSION)/examples
	cp examples/*.tcl $(NAME)-$(VERSION)/examples
	tar -czvvf $(NAME)-$(VERSION).tar.gz $(NAME)-$(VERSION)
	rm -rvf $(NAME)-$(VERSION)

#############################################
version: Doxyfile tclmpi.tcl pkgIndex.tcl

Doxyfile: Doxyfile.in
	sed -e s,@VERSION@,$(VERSION),g $< > $@

tclmpi.tcl: tclmpi.tcl.in pkgIndex.tcl
	sed -e s,@VERSION@,$(VERSION),g $< > $@

pkgIndex.tcl: pkgIndex.tcl.in
	sed -e s,@VERSION@,$(VERSION),g $< > $@

#############################################
.PHONY: default clean check doc all dynamic static check-static version tar
.SUFFIXES:

