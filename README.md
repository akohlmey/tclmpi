The TclMPI package  contains software that wraps  an MPI library for Tcl
and allows MPI  calls to  be  used from  Tcl scripts.  This code can  be
compiled as   a  shared object   to   be loaded   into  an existing  Tcl
interpreter  or as a  standalone TclMPI interpreter. In combination with
some additional  bundled   Tcl script  code,  additional  commands   are
provided that   allow to run   Tcl scripts in  parallel via  "mpirun" or
"mpiexec" similar to C, C++ or Fortran programs.

## Homepage:

The main author of this package is Axel Kohlmeyer  and you can reach him
at <akohlmey@gmail.com>.  The  official  homepage  for this  project  is
[https://akohlmey.github.io/tclmpi/](https://akohlmey.github.io/tclmpi/)
and development is [hosted on GitHub](https://github.com/akohlmey/tclmpi/).

For compilation   and  installation instructions,  please see   the file
INSTALL.   Detailed documentation is   available online from the project
home page and [as a PDF file in the source package](https://github.com/akohlmey/tclmpi/blob/master/tclmpi_docs.pdf).

## Test Status:

[![Linux](https://github.com/akohlmey/tclmpi/actions/workflows/unittest-linux.yml/badge.svg?branch=master)](https://github.com/akohlmey/tclmpi/actions/workflows/unittest-linux.yml)
[![macOS](https://github.com/akohlmey/tclmpi/actions/workflows/unittest-macos.yml/badge.svg?branch=master)](https://github.com/akohlmey/tclmpi/actions/workflows/unittest-macos.yml)
[![CodeQL](https://github.com/akohlmey/tclmpi/actions/workflows/codeql-analysis.yml/badge.svg?branch=master)](https://github.com/akohlmey/tclmpi/actions/workflows/codeql-analysis.yml)

## Citing:

You can cite TclMPI as:

Axel Kohlmeyer. (2021). TclMPI: Release 1.1 [Data set]. Zenodo. [![DOI](https://www.zenodo.org/badge/4368856.svg)](https://www.zenodo.org/badge/latestdoi/4368856)

## Acknowledgements:

Thanks to Arjen Markus and Chris MacDermaid  for encouragement and (lots
of) constructive criticism, that has  helped enourmously  to develop the
package  from a crazy idea to its current  level.  Thanks to  Alex Baker
for motivating me to convert to using CMake as build system which makes
building TclMPI natively on Windows much easier.

A special thanks also
goes to Karolina Sarnowska-Upton and  Andrew Grimshaw that allowed me to
use TclMPI as an example in their MPI portability study, which helped to
find quite a few bugs and  resolve several portability issues before the
code was hitting the real world.

